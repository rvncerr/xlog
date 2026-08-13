#include "xlog.h"
#include "crc32c.h"
#include "endian.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

/* The header size literal is the on-disk format; these verify the code's
   assumptions about it. */
_Static_assert(XLOG_HEADER_SIZE == 2 * sizeof(uint32_t),
               "header is a 32-bit size plus a 32-bit CRC32C");
_Static_assert(XLOG_MAX_RECORD_SIZE + XLOG_HEADER_SIZE <= SSIZE_MAX,
               "record cap must fit a single read/write");

struct xlog_writer {
    int fd;
    uint32_t max_record_size;
    int flags;
};

struct xlog_reader {
    int fd;
    uint32_t max_record_size;
    int flags;
};

/* Returns n on success, 0..n-1 on EOF, -1 on I/O error (errno preserved). */
static ssize_t xlog_readall(int fd, void *buf, size_t n) {
    uint8_t *p = buf;
    while(n > 0) {
        ssize_t r = read(fd, p, n);
        if(r < 0) {
            if(errno == EINTR) continue;
            return -1;
        }
        if(r == 0) return p - (uint8_t *)buf;
        p += r;
        n -= r;
    }
    return p - (uint8_t *)buf;
}

/* Durable flush. On macOS fsync only hands data to the drive's write cache,
   so F_FULLFSYNC is needed to force a media flush; volumes that do not
   implement it (SMB, NFS, some virtual devices) report it as unsupported
   and fall back to fsync. Real failures such as EIO propagate. */
static int xlog_sync(int fd) {
#ifdef __APPLE__
    if(fcntl(fd, F_FULLFSYNC) == 0) return 0;
    if(errno == ENOTSUP || errno == EOPNOTSUPP || errno == ENOTTY ||
       errno == ENODEV || errno == EINVAL)
        return fsync(fd);
    return -1;
#else
    return fdatasync(fd);
#endif
}

xlog_writer *xlog_writer_open_ex(const char *path, uint32_t max_record_size, int flags) {
    if(max_record_size == 0 || max_record_size > XLOG_MAX_RECORD_SIZE) {
        errno = EINVAL;
        return NULL;
    }

    xlog_writer *w = malloc(sizeof(xlog_writer));
    if(!w) return NULL;

    w->fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if(w->fd < 0) {
        free(w);
        return NULL;
    }

    w->max_record_size = max_record_size;
    w->flags = flags;
    return w;
}

xlog_writer *xlog_writer_open(const char *path) {
    return xlog_writer_open_ex(path, XLOG_MAX_RECORD_SIZE, 0);
}

int xlog_writer_commit(xlog_writer *w, const void *buf, size_t sz) {
    if(sz == 0 || sz > w->max_record_size) return XLOG_ERR_SIZE;

    uint8_t hdr[XLOG_HEADER_SIZE];
    put_le32(hdr, (uint32_t)sz);
    put_le32(hdr + 4, crc32c(0, buf, sz));

    struct iovec iov[2] = {
        { .iov_base = hdr,        .iov_len = XLOG_HEADER_SIZE },
        { .iov_base = (void *)buf, .iov_len = sz },
    };

    ssize_t total = XLOG_HEADER_SIZE + sz;
    ssize_t written;
    while((written = writev(w->fd, iov, 2)) < 0 && errno == EINTR)
        ;
    if(written != total)
        return XLOG_ERR_IO;

    if(!(w->flags & XLOG_NOSYNC)) {
        if(xlog_sync(w->fd) < 0)
            return XLOG_ERR_SYNC;
    }

    return 0;
}

int xlog_writer_close(xlog_writer *w) {
    if(!w) return 0;
    int rc = close(w->fd) < 0 ? XLOG_ERR_IO : 0;
    free(w);
    return rc;
}

xlog_reader *xlog_reader_open_ex(const char *path, uint32_t max_record_size, int flags) {
    if(max_record_size == 0 || max_record_size > XLOG_MAX_RECORD_SIZE) {
        errno = EINVAL;
        return NULL;
    }

    xlog_reader *r = malloc(sizeof(xlog_reader));
    if(!r) return NULL;

    r->fd = open(path, O_RDONLY | O_CLOEXEC);
    if(r->fd < 0) {
        free(r);
        return NULL;
    }

    r->max_record_size = max_record_size;
    r->flags = flags;
    return r;
}

xlog_reader *xlog_reader_open(const char *path) {
    return xlog_reader_open_ex(path, XLOG_MAX_RECORD_SIZE, 0);
}

int xlog_reader_reset(xlog_reader *r) {
    return lseek(r->fd, 0, SEEK_SET) < 0 ? XLOG_ERR_IO : 0;
}

/* pos is the offset of the record start, used to rewind on a short read at
   EOF (torn tail or racing a writer) so the caller can retry after the
   record is completed. */
static ssize_t xlog_decode(xlog_reader *r, void *buf, size_t cap, off_t pos) {
    uint8_t hdr[XLOG_HEADER_SIZE];
    ssize_t rd = xlog_readall(r->fd, hdr, XLOG_HEADER_SIZE);
    if(rd < 0) return XLOG_ERR_IO;
    if(rd != XLOG_HEADER_SIZE) {
        if(rd > 0 && lseek(r->fd, pos, SEEK_SET) < 0) return XLOG_ERR_IO;
        return XLOG_EOF;
    }

    uint32_t size = get_le32(hdr);
    uint32_t checksum = get_le32(hdr + 4);

    if(size == 0 || size > r->max_record_size)
        return XLOG_ERR_SIZE;

    if(size > cap) {
        if(lseek(r->fd, pos, SEEK_SET) < 0) return XLOG_ERR_IO;
        return XLOG_ERR_TOOBIG;
    }

    rd = xlog_readall(r->fd, buf, size);
    if(rd < 0) return XLOG_ERR_IO;
    if(rd != (ssize_t)size) {
        (void)lseek(r->fd, pos, SEEK_SET);
        return XLOG_ERR_TRUNCATED;
    }

    if(checksum != crc32c(0, buf, size))
        return XLOG_ERR_CRC;

    return size;
}

ssize_t xlog_reader_next(xlog_reader *r, void *buf, size_t cap) {
    int scanning = 0;

    for(;;) {
        off_t pos = lseek(r->fd, 0, SEEK_CUR);
        if(pos < 0) return XLOG_ERR_IO;
        ssize_t rc = xlog_decode(r, buf, cap, pos);

        if(rc >= 0) { return rc; }
        if(rc == XLOG_EOF) { return XLOG_EOF; }

        if(rc == XLOG_ERR_CRC && !scanning && (r->flags & XLOG_SKIP_CORRUPT))
            continue;

        /* A valid record that merely exceeds the caller's buffer must never
           be skip-scanned away. During a scan, though, candidate sizes are
           untrusted, so an unverifiable oversized candidate is stepped over
           below like any other non-record. */
        if(rc == XLOG_ERR_TOOBIG && !scanning)
            return rc;

        /* Truncation means the file ends mid-record: there is nothing
           beyond it to scan for. During a scan, though, a candidate whose
           size runs past EOF is just a non-record; keep stepping in case a
           real record starts closer to the end. */
        if(rc == XLOG_ERR_TRUNCATED && !scanning)
            return rc;

        if(r->flags & XLOG_SKIP_BADSIZE) {
            scanning = 1;
            if(lseek(r->fd, pos + 1, SEEK_SET) < 0) return XLOG_ERR_IO;
            continue;
        }

        return rc;
    }
}

void xlog_reader_close(xlog_reader *r) {
    if(!r) return;
    close(r->fd);
    free(r);
}

const char *xlog_strerror(int code) {
    switch(code) {
    case XLOG_EOF:      return "end of log";
    case XLOG_ERR_IO:   return "I/O error";
    case XLOG_ERR_CRC:  return "checksum mismatch";
    case XLOG_ERR_SIZE: return "invalid record size";
    case XLOG_ERR_SYNC: return "sync failed, data written but not durable";
    case XLOG_ERR_TOOBIG: return "record larger than caller buffer";
    case XLOG_ERR_TRUNCATED: return "record truncated at end of log";
    default:            return "unknown error";
    }
}
