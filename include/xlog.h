#ifndef XLOG_H
#define XLOG_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define XLOG_EOF       0
#define XLOG_ERR_IO   -1
#define XLOG_ERR_CRC  -2
#define XLOG_ERR_SIZE -3
#define XLOG_ERR_MEM  -4

typedef struct xlog_reader xlog_reader_t;
typedef struct xlog_writer xlog_writer_t;

xlog_reader_t *xlog_reader_open(const char *path);
xlog_reader_t *xlog_reader_open_ex(const char *path, uint32_t max_record_size);
void xlog_reader_reset(xlog_reader_t *r);
ssize_t xlog_reader_next(xlog_reader_t *r, void **buf);
void xlog_reader_close(xlog_reader_t *r);

xlog_writer_t *xlog_writer_open(const char *path);
xlog_writer_t *xlog_writer_open_ex(const char *path, uint32_t max_record_size);
int xlog_writer_commit(xlog_writer_t *w, const void *buf, size_t sz);
void xlog_writer_close(xlog_writer_t *w);

#endif // XLOG_H
