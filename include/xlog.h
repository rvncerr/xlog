#ifndef XLOG_H
#define XLOG_H

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

/* Negative return codes are errors; see xlog_strerror(). errno is meaningful
   only after XLOG_ERR_IO or XLOG_ERR_SYNC (a failed system call), with one
   exception: a short writev in xlog_writer_commit returns XLOG_ERR_IO with
   errno unchanged. */
#define XLOG_EOF            0
#define XLOG_ERR_IO        -1
#define XLOG_ERR_CRC       -2
#define XLOG_ERR_SIZE      -3  /* size field is zero or exceeds max_record_size */
#define XLOG_ERR_SYNC      -4
#define XLOG_ERR_TOOBIG    -5  /* record exceeds caller's buffer; reader stays
                                  on the record, retry with a larger buffer */
#define XLOG_ERR_TRUNCATED -6  /* file ends mid-record (torn tail after a crash
                                  or a writer still appending); reader stays on
                                  the record */

/* Writer flag: skip the per-commit sync. Records reach the file but become
   durable only when the OS decides; nothing in the library syncs afterwards,
   not even xlog_writer_close. */
#define XLOG_NOSYNC       (1 << 0)

/* Reader flags: salvage options for a damaged log, off by default. See
   xlog_reader_next for what each one steps over. Flags that do not apply to
   the handle being opened are ignored. */
#define XLOG_SKIP_CORRUPT (1 << 1)
#define XLOG_SKIP_BADSIZE (1 << 2)

/* On-disk record header: 4-byte little-endian payload size + 4-byte CRC32C. */
#define XLOG_HEADER_SIZE 8

/* Hard ceiling on record size; the _ex opens reject a larger
   max_record_size. A commit must complete in one writev call, and Linux
   transfers at most
   INT_MAX & PAGE_MASK bytes per call — the mask here assumes 64 KiB pages,
   the largest in common use. This also keeps header + payload within
   ssize_t on 32-bit platforms (static assert in xlog.c). */
#define XLOG_MAX_RECORD_SIZE ((INT_MAX & ~0xffffu) - XLOG_HEADER_SIZE)

#ifdef XLOG_BUILDING
    #define XLOG_API __attribute__((visibility("default")))
#else
    #define XLOG_API
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define XLOG_NODISCARD __attribute__((warn_unused_result))
    #define XLOG_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
    #define XLOG_NODISCARD
    #define XLOG_NONNULL(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xlog_reader xlog_reader;
typedef struct xlog_writer xlog_writer;

/* Handles are opaque and are not thread-safe: each owns one file descriptor
   and one file position, and the library adds no locking. Separate handles,
   even on the same path, are independent.

   Pointer arguments must not be NULL. The two close functions are the
   exception — they accept NULL so cleanup paths need no guard. */

/* Opens path for reading, positioned at the first record.

   Returns NULL on failure with errno set by open(2): ENOENT if the log does
   not exist, EACCES, and so on. xlog_reader_open_ex also fails with EINVAL
   if max_record_size is 0 or above XLOG_MAX_RECORD_SIZE, and accepts
   XLOG_SKIP_CORRUPT and XLOG_SKIP_BADSIZE. The plain form uses
   XLOG_MAX_RECORD_SIZE and no flags.

   max_record_size is the reader's corruption bound, not a buffer size: a
   header claiming more is rejected as XLOG_ERR_SIZE instead of being
   trusted. A record too large for the caller's buffer is a separate case,
   reported as XLOG_ERR_TOOBIG. */
XLOG_API xlog_reader *xlog_reader_open(const char *path) XLOG_NONNULL(1);
XLOG_API xlog_reader *xlog_reader_open_ex(const char *path, uint32_t max_record_size, int flags) XLOG_NONNULL(1);

/* Rewinds to the first record, which is also how to recover a reader whose
   position an error left unspecified. Returns 0, or XLOG_ERR_IO with errno
   set by lseek(2). */
XLOG_API XLOG_NODISCARD int xlog_reader_reset(xlog_reader *r) XLOG_NONNULL(1);

/* Reads the next record into buf, which must have room for cap bytes.

   Returns the record size (> 0), XLOG_EOF (0) at the end of the log, or a
   negative error code. Where an error leaves the read position decides
   whether the caller can carry on:

     XLOG_ERR_TOOBIG     the record does not fit in cap bytes. The position
                         is unchanged; retry with a larger buffer. The size
                         needed is not reported, so grow the buffer
                         geometrically or go straight to max_record_size.
     XLOG_ERR_TRUNCATED  the log ends mid-record — a tail torn by a crash, or
                         a writer that has not finished appending. The
                         position is unchanged, so a tailing reader can retry
                         the same call once the record is complete.
     XLOG_ERR_CRC        the payload failed its checksum. The reader has
                         advanced past the record, so the next call resumes
                         at the following one; XLOG_SKIP_CORRUPT does that
                         automatically.
     XLOG_ERR_SIZE       the size field is 0 or above max_record_size, so the
                         record's extent is unknown and the reader is left
                         just past the header, not at a record boundary.
                         Stop, call xlog_reader_reset, or open with
                         XLOG_SKIP_BADSIZE to have the reader scan forward
                         for the next valid record.
     XLOG_ERR_IO         a read or seek failed; errno is set and the position
                         is unspecified.

   Reaching XLOG_EOF is not sticky: a later call picks up records a writer
   has appended since. */
XLOG_API XLOG_NODISCARD ssize_t xlog_reader_next(xlog_reader *r, void *buf, size_t cap) XLOG_NONNULL(1, 2);

/* Releases the reader; accepts NULL. The descriptor is closed but the result
   is not reported, as a reader has nothing buffered to lose. The handle must
   not be used afterwards. */
XLOG_API void xlog_reader_close(xlog_reader *r);

/* Opens path for appending, creating it 0644 (before umask) if it does not
   exist.

   Returns NULL on failure with errno set by open(2). xlog_writer_open_ex
   also fails with EINVAL if max_record_size is 0 or above
   XLOG_MAX_RECORD_SIZE, and accepts XLOG_NOSYNC. The plain form uses
   XLOG_MAX_RECORD_SIZE and syncs.

   Creating the log also syncs the parent directory once, so the file itself
   survives a crash and not just its contents. That step is skipped under
   XLOG_NOSYNC, and is best effort: a directory that cannot be opened or
   synced is not an error. */
XLOG_API xlog_writer *xlog_writer_open(const char *path) XLOG_NONNULL(1);
XLOG_API xlog_writer *xlog_writer_open_ex(const char *path, uint32_t max_record_size, int flags) XLOG_NONNULL(1);

/* Appends one record: an XLOG_HEADER_SIZE header plus the sz bytes at buf,
   in a single writev(2).

   Returns 0 on success, and unless XLOG_NOSYNC is set that means the record
   is on stable storage. Errors:

     XLOG_ERR_SIZE   sz is 0 or above the writer's max_record_size. Nothing
                     is written and buf is not read.
     XLOG_ERR_IO     the write failed or completed only partially. A partial
                     write leaves a torn record at the end of the log, which
                     readers report as XLOG_ERR_TRUNCATED.
     XLOG_ERR_SYNC   the record was written but could not be flushed: it is
                     in the file and readable, just not known to be durable. */
XLOG_API XLOG_NODISCARD int xlog_writer_commit(xlog_writer *w, const void *buf, size_t sz) XLOG_NONNULL(1, 2);

/* Closes the log; accepts NULL and returns 0 for it. No sync happens here —
   each commit already synced, unless XLOG_NOSYNC was set, in which case
   durability is entirely the caller's business.

   Returns XLOG_ERR_IO if close(2) failed, with errno set. The writer is
   freed either way, so it must not be reused or closed twice. */
XLOG_API XLOG_NODISCARD int xlog_writer_close(xlog_writer *w);

/* Returns a short description of any xlog return code, including XLOG_EOF
   and unrecognized values. Never NULL; the string is static storage and must
   not be freed. */
XLOG_API const char *xlog_strerror(int code);

#ifdef __cplusplus
}
#endif

#endif // XLOG_H
