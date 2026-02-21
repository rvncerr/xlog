#include "xlog.h"
#include "crc32c.h"
#include "endian.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define XLOG_HEADER_SIZE 8

struct xlog_writer {
    FILE *fd;
    uint32_t max_record_size;
    int flags;
};

struct xlog_reader {
    FILE *fd;
    uint32_t max_record_size;
};

static void xlog_lock(int fd) {
    struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET };
    fcntl(fd, F_SETLKW, &fl);
}

static void xlog_unlock(int fd) {
    struct flock fl = { .l_type = F_UNLCK, .l_whence = SEEK_SET };
    fcntl(fd, F_SETLK, &fl);
}

xlog_writer_t *xlog_writer_open_ex(const char *path, uint32_t max_record_size, int flags) {
    xlog_writer_t *w = malloc(sizeof(xlog_writer_t));
    if(!w) return NULL;

    w->fd = fopen(path, "ab");
    if(w->fd == NULL) {
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

    int fd = fileno(w->fd);
    xlog_lock(fd);
    fseek(w->fd, 0, SEEK_END);
    if(fwrite(hdr, XLOG_HEADER_SIZE, 1, w->fd) != 1 ||
       fwrite(buf, sz, 1, w->fd) != 1) {
        xlog_unlock(fd);
        return XLOG_ERR_IO;
    }
    fflush(w->fd);
    if(!(w->flags & XLOG_NOSYNC))
        fdatasync(fd);
    xlog_unlock(fd);
    return 0;
}

void xlog_writer_close(xlog_writer_t *w) {
    fclose(w->fd);
    free(w);
}

xlog_reader_t *xlog_reader_open_ex(const char *path, uint32_t max_record_size) {
    xlog_reader_t *r = malloc(sizeof(xlog_reader_t));
    if(!r) return NULL;

    r->fd = fopen(path, "rb");
    if(r->fd == NULL) {
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
    fseek(r->fd, 0, SEEK_SET);
}

ssize_t xlog_reader_next(xlog_reader_t *r, void **buf) {
    uint8_t hdr[XLOG_HEADER_SIZE];
    if(fread(hdr, XLOG_HEADER_SIZE, 1, r->fd) != 1)
        return XLOG_EOF;

    uint32_t size = get_le32(hdr);
    uint32_t checksum = get_le32(hdr + 4);

    if(size == 0 || size > r->max_record_size)
        return XLOG_ERR_SIZE;

    *buf = malloc(size);
    if(!*buf)
        return XLOG_ERR_MEM;

    if(fread(*buf, size, 1, r->fd) != 1) {
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
    fclose(r->fd);
    free(r);
}
