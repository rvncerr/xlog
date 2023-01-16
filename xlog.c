#include "xlog.h"
#include "crc32c.h"

xlog_writer_t *xlog_writer_open(const char *path) {
    xlog_writer_t *w = malloc(sizeof(xlog_writer_t));
    if(!w) return NULL;

    w->fd = fopen(path, "wb");
    if(w->fd == NULL) {
        free(w);
        return NULL;
    }

    return w;
}

void xlog_writer_commit(xlog_writer_t *w, void *buf, size_t sz) {
    xlog_header_t *h = malloc(sizeof(xlog_header_t));
    h->size = sz;
    h->checksum = crc32c(0, buf, sz);
    flock(fileno(w->fd), LOCK_EX);
    fseek(w->fd, 0, SEEK_END);
    fwrite(h, sizeof(xlog_header_t), 1, w->fd);
    fwrite(buf, sz, 1, w->fd);
    fflush(w->fd);
    flock(fileno(w->fd), LOCK_UN);
    free(h);
}

void xlog_writer_close(xlog_writer_t *w) {
    fclose(w->fd);
    free(w);
}

xlog_reader_t *xlog_reader_open(const char *path) {
    xlog_reader_t *r = malloc(sizeof(xlog_reader_t));
    if(!r) return NULL;

    r->fd = fopen(path, "rb");
    if(r->fd == NULL) {
        free(r);
        return NULL;
    }

    return r;
}

void xlog_reader_reset(xlog_reader_t *r) {
    flock(fileno(r->fd), LOCK_SH);
    fseek(r->fd, 0, SEEK_SET);
    flock(fileno(r->fd), LOCK_UN);
}

size_t xlog_reader_next(xlog_reader_t *r, void **buf) {
    xlog_header_t h;
    flock(fileno(r->fd), LOCK_SH);
    if(fread(&h, sizeof(h), 1, r->fd) != 1) {
        flock(fileno(r->fd), LOCK_UN);
        return 0;
    }

    *buf = malloc(h.size);
    if(!*buf) return 0;

    if(fread(*buf, h.size, 1, r->fd) != 1) {
        free(*buf);
        flock(fileno(r->fd), LOCK_UN);
        return 0;
    }

    flock(fileno(r->fd), LOCK_UN);

    if(h.checksum != crc32c(0, *buf, h.size)) {
        free(*buf);
        return 0;
    }

    return h.size;
}

void xlog_reader_close(xlog_reader_t *r) {
    fclose(r->fd);
    free(r);
}
