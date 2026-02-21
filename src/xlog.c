#include "xlog.h"
#include "crc32c.h"

xlog_writer_t *xlog_writer_open_ex(const char *path, uint32_t max_record_size) {
    xlog_writer_t *w = malloc(sizeof(xlog_writer_t));
    if(!w) return NULL;

    w->fd = fopen(path, "ab");
    if(w->fd == NULL) {
        free(w);
        return NULL;
    }

    w->max_record_size = max_record_size;
    return w;
}

xlog_writer_t *xlog_writer_open(const char *path) {
    return xlog_writer_open_ex(path, UINT32_MAX);
}

int xlog_writer_commit(xlog_writer_t *w, void *buf, size_t sz) {
    if(sz == 0 || sz > w->max_record_size) return XLOG_ERR_SIZE;

    xlog_header_t h;
    h.size = sz;
    h.checksum = crc32c(0, buf, sz);
    flock(fileno(w->fd), LOCK_EX);
    fseek(w->fd, 0, SEEK_END);
    if(fwrite(&h, sizeof(xlog_header_t), 1, w->fd) != 1 ||
       fwrite(buf, sz, 1, w->fd) != 1) {
        flock(fileno(w->fd), LOCK_UN);
        return XLOG_ERR_IO;
    }
    fflush(w->fd);
    flock(fileno(w->fd), LOCK_UN);
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
    flock(fileno(r->fd), LOCK_SH);
    fseek(r->fd, 0, SEEK_SET);
    flock(fileno(r->fd), LOCK_UN);
}

ssize_t xlog_reader_next(xlog_reader_t *r, void **buf) {
    xlog_header_t h;
    flock(fileno(r->fd), LOCK_SH);
    if(fread(&h, sizeof(h), 1, r->fd) != 1) {
        flock(fileno(r->fd), LOCK_UN);
        return XLOG_EOF;
    }

    if(h.size == 0 || h.size > r->max_record_size) {
        flock(fileno(r->fd), LOCK_UN);
        return XLOG_ERR_SIZE;
    }

    *buf = malloc(h.size);
    if(!*buf) {
        flock(fileno(r->fd), LOCK_UN);
        return XLOG_ERR_MEM;
    }

    if(fread(*buf, h.size, 1, r->fd) != 1) {
        free(*buf);
        flock(fileno(r->fd), LOCK_UN);
        return XLOG_ERR_IO;
    }

    flock(fileno(r->fd), LOCK_UN);

    if(h.checksum != crc32c(0, *buf, h.size)) {
        free(*buf);
        return XLOG_ERR_CRC;
    }

    return h.size;
}

void xlog_reader_close(xlog_reader_t *r) {
    fclose(r->fd);
    free(r);
}
