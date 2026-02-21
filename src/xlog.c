#include "xlog.h"
#include "crc32c.h"
#include "endian.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>

#define XLOG_HEADER_SIZE 8

struct xlog_writer {
    int fd;
    uint32_t max_record_size;
    int flags;
};

struct xlog_reader {
    int fd;
    uint32_t max_record_size;
};

static ssize_t xlog_readall(int fd, void *buf, size_t n) {
    uint8_t *p = buf;
    while(n > 0) {
        ssize_t r = read(fd, p, n);
        if(r <= 0) return r;
        p += r;
        n -= r;
    }
    return p - (uint8_t *)buf;
}

xlog_writer_t *xlog_writer_open_ex(const char *path, uint32_t max_record_size, int flags) {
    xlog_writer_t *w = malloc(sizeof(xlog_writer_t));
    if(!w) return NULL;

    w->fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(w->fd < 0) {
        free(w);
        return NULL;
    }

    w->max_record_size = max_record_size;
    w->flags = flags;
    return w;
}

xlog_writer_t *xlog_writer_open(const char *path) {
    return xlog_writer_open_ex(path, UINT32_MAX, 0);
}

int xlog_writer_commit(xlog_writer_t *w, const void *buf, size_t sz) {
    if(sz == 0 || sz > w->max_record_size) return XLOG_ERR_SIZE;

    uint8_t hdr[XLOG_HEADER_SIZE];
    put_le32(hdr, (uint32_t)sz);
    put_le32(hdr + 4, crc32c(0, buf, sz));

    struct iovec iov[2] = {
        { .iov_base = hdr,        .iov_len = XLOG_HEADER_SIZE },
        { .iov_base = (void *)buf, .iov_len = sz },
    };

    ssize_t total = XLOG_HEADER_SIZE + sz;
    if(writev(w->fd, iov, 2) != total)
        return XLOG_ERR_IO;

    if(!(w->flags & XLOG_NOSYNC))
        fdatasync(w->fd);

    return 0;
}

void xlog_writer_close(xlog_writer_t *w) {
    close(w->fd);
    free(w);
}

xlog_reader_t *xlog_reader_open_ex(const char *path, uint32_t max_record_size) {
    xlog_reader_t *r = malloc(sizeof(xlog_reader_t));
    if(!r) return NULL;

    r->fd = open(path, O_RDONLY);
    if(r->fd < 0) {
        free(r);
        return NULL;
    }

    r->max_record_size = max_record_size;
    return r;
}

xlog_reader_t *xlog_reader_open(const char *path) {
    return xlog_reader_open_ex(path, UINT32_MAX);
}

void xlog_reader_reset(xlog_reader_t *r) {
    lseek(r->fd, 0, SEEK_SET);
}

ssize_t xlog_reader_next(xlog_reader_t *r, void **buf) {
    uint8_t hdr[XLOG_HEADER_SIZE];
    if(xlog_readall(r->fd, hdr, XLOG_HEADER_SIZE) != XLOG_HEADER_SIZE)
        return XLOG_EOF;

    uint32_t size = get_le32(hdr);
    uint32_t checksum = get_le32(hdr + 4);

    if(size == 0 || size > r->max_record_size)
        return XLOG_ERR_SIZE;

    *buf = malloc(size);
    if(!*buf)
        return XLOG_ERR_MEM;

    if(xlog_readall(r->fd, *buf, size) != (ssize_t)size) {
        free(*buf);
        return XLOG_ERR_IO;
    }

    if(checksum != crc32c(0, *buf, size)) {
        free(*buf);
        return XLOG_ERR_CRC;
    }

    return size;
}

void xlog_reader_close(xlog_reader_t *r) {
    close(r->fd);
    free(r);
}
