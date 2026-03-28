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

#define XLOG_NOSYNC       (1 << 0)
#define XLOG_SKIP_CORRUPT (1 << 1)
#define XLOG_SKIP_BADSIZE (1 << 2)

#ifdef XLOG_BUILDING
    #define XLOG_API __attribute__((visibility("default")))
#else
    #define XLOG_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xlog_reader xlog_reader;
typedef struct xlog_writer xlog_writer;

XLOG_API xlog_reader *xlog_reader_open(const char *path);
XLOG_API xlog_reader *xlog_reader_open_ex(const char *path, uint32_t max_record_size, int flags);
XLOG_API void xlog_reader_reset(xlog_reader *r);
XLOG_API ssize_t xlog_reader_next(xlog_reader *r, void **buf);
XLOG_API void xlog_reader_close(xlog_reader *r);

XLOG_API xlog_writer *xlog_writer_open(const char *path);
XLOG_API xlog_writer *xlog_writer_open_ex(const char *path, uint32_t max_record_size, int flags);
XLOG_API int xlog_writer_commit(xlog_writer *w, const void *buf, size_t sz);
XLOG_API void xlog_writer_close(xlog_writer *w);

XLOG_API const char *xlog_strerror(int code);

#ifdef __cplusplus
}
#endif

#endif // XLOG_H
