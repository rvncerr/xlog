#ifndef XLOG_H
#define XLOG_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t size;
    uint32_t checksum;
} xlog_header_t;
#pragma pack(pop)

typedef struct {
    FILE *fd;
} xlog_reader_t;

xlog_reader_t *xlog_reader_open(const char *path);
void xlog_reader_reset(xlog_reader_t *r);
size_t xlog_reader_next(xlog_reader_t *r, void **buf);
void xlog_reader_close(xlog_reader_t *r);

typedef struct {
    FILE *fd;
} xlog_writer_t;

xlog_writer_t *xlog_writer_open(const char *path);
void xlog_writer_commit(xlog_writer_t *w, void *buf, size_t sz);
void xlog_writer_close(xlog_writer_t *w);

#endif // XLOG_H
