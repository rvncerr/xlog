#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xlog.h"
#include "crc32c.h"

static void test_crc32c(void) {
    uint8_t t32_0[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t t32_1[32] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    uint8_t t32_i[32] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    uint8_t t32_d[32] = {
            0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
            0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
            0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
            0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
    };
    CU_ASSERT_EQUAL(crc32c(0, t32_0, 32), 0x8a9136aa);
    CU_ASSERT_EQUAL(crc32c(0, t32_1, 32), 0x62a8ab43);
    CU_ASSERT_EQUAL(crc32c(0, t32_i, 32), 0x46dd794e);
    CU_ASSERT_EQUAL(crc32c(0, t32_d, 32), 0x113fdb5c);
}

static void test_xlog_basic(void) {
    unlink("test.xlog");

    char *wbuf = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                "sed do eiusmod tempor incididunt ut labore et dolore magna "
                "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
                "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
                "Duis aute irure dolor in reprehenderit in voluptate velit "
                "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
                "occaecat cupidatat non proident, sunt in culpa qui officia "
                "deserunt mollit anim id est laborum.";

    xlog_writer_t *w = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, wbuf, strlen(wbuf) + 1), 0);
    xlog_writer_close(w);

    char *rbuf;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    ssize_t sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, (ssize_t)(strlen(rbuf) + 1));
    CU_ASSERT_STRING_EQUAL_FATAL(wbuf, rbuf);
    free(rbuf);
    xlog_reader_close(r);
}

static void test_xlog_multi(void) {
    unlink("test.xlog");

    xlog_writer_t *w = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);
    for(int i = 0; i < 1000; i++) {
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_EQUAL(xlog_writer_commit(w, buf, strlen(buf) + 1), 0);
    }
    xlog_writer_close(w);

    char *rbuf;
    ssize_t sz;

    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    for(int i = 0; i < 1000; i++) {
        sz = xlog_reader_next(r, (void **)&rbuf);
        CU_ASSERT_EQUAL_FATAL(sz, (ssize_t)(strlen(rbuf) + 1));
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_STRING_EQUAL_FATAL(buf, rbuf);
        free(rbuf);
    }

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);

    xlog_reader_reset(r);

    for(int i = 0; i < 1000; i++) {
        sz = xlog_reader_next(r, (void **)&rbuf);
        CU_ASSERT_EQUAL_FATAL(sz, (ssize_t)(strlen(rbuf) + 1));
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_STRING_EQUAL_FATAL(buf, rbuf);
        free(rbuf);
    }

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);

    xlog_reader_close(r);
}

static void test_xlog_reopen_append(void) {
    unlink("test.xlog");

    xlog_writer_t *w1 = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w1);
    CU_ASSERT_EQUAL(xlog_writer_commit(w1, "first", 6), 0);
    xlog_writer_close(w1);

    xlog_writer_t *w2 = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w2);
    CU_ASSERT_EQUAL(xlog_writer_commit(w2, "second", 7), 0);
    xlog_writer_close(w2);

    char *rbuf;
    ssize_t sz;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 6);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "first");
    free(rbuf);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 7);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "second");
    free(rbuf);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);

    xlog_reader_close(r);
}

static void test_xlog_max_record_size(void) {
    unlink("test.xlog");

    xlog_writer_t *w = xlog_writer_open_ex("test.xlog", 8, XLOG_NOSYNC);
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);

    CU_ASSERT_EQUAL(xlog_writer_commit(w, "short", 6), 0);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "this is too long", 17), XLOG_ERR_SIZE);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "ok", 3), 0);
    xlog_writer_close(w);

    char *rbuf;
    ssize_t sz;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 6);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "short");
    free(rbuf);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 3);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "ok");
    free(rbuf);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);
    xlog_reader_close(r);

    r = xlog_reader_open_ex("test.xlog", 4, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_ERR_SIZE);

    xlog_reader_close(r);
}

static void test_xlog_errors(void) {
    unlink("test.xlog");

    /* Writer: zero-size commit returns XLOG_ERR_SIZE */
    xlog_writer_t *w = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "data", 0), XLOG_ERR_SIZE);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "hello", 6), 0);
    xlog_writer_close(w);

    /* Reader: corrupt checksum returns XLOG_ERR_CRC */
    FILE *f = fopen("test.xlog", "r+b");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    fseek(f, 8 /* sizeof(xlog_header_t) */, SEEK_SET);
    uint8_t garbage = 0xFF;
    fwrite(&garbage, 1, 1, f);
    fclose(f);

    char *rbuf;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    ssize_t sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_ERR_CRC);
    xlog_reader_close(r);

    /* Reader: EOF on empty file */
    unlink("test.xlog");
    f = fopen("test.xlog", "wb");
    fclose(f);
    r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);
    xlog_reader_close(r);

    /* Reader: truncated payload returns XLOG_ERR_IO */
    unlink("test.xlog");
    uint8_t fake_header[8] = {
        100, 0, 0, 0,  /* size = 100 (little-endian) */
          0, 0, 0, 0,  /* checksum = 0 */
    };
    f = fopen("test.xlog", "wb");
    CU_ASSERT_PTR_NOT_NULL_FATAL(f);
    fwrite(fake_header, sizeof(fake_header), 1, f);
    fwrite("short", 5, 1, f);
    fclose(f);
    r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_ERR_IO);
    xlog_reader_close(r);
}

static void test_xlog_skip_corrupt(void) {
    unlink("test.xlog");

    xlog_writer_t *w = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "aaa", 4), 0);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "bbb", 4), 0);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "ccc", 4), 0);
    xlog_writer_close(w);

    /* Corrupt the payload of the second record */
    int fd = open("test.xlog", O_RDWR);
    CU_ASSERT_FATAL(fd >= 0);
    /* record 1: 8 hdr + 4 data = 12 bytes, record 2 header at offset 12, payload at 20 */
    lseek(fd, 20, SEEK_SET);
    uint8_t garbage = 0xFF;
    write(fd, &garbage, 1);
    close(fd);

    /* Without XLOG_SKIP_CORRUPT: stops at corrupt record */
    char *rbuf;
    ssize_t sz;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 4);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "aaa");
    free(rbuf);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_ERR_CRC);
    xlog_reader_close(r);

    /* With XLOG_SKIP_CORRUPT: skips corrupt, reads third record */
    r = xlog_reader_open_ex("test.xlog", UINT32_MAX, XLOG_SKIP_CORRUPT);
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 4);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "aaa");
    free(rbuf);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 4);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "ccc");
    free(rbuf);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);
    xlog_reader_close(r);
}

static void test_xlog_skip_badsize(void) {
    unlink("test.xlog");

    /* Write: [aaa] [bbb] [ccc] */
    xlog_writer_t *w = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "aaa", 4), 0);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "bbb", 4), 0);
    CU_ASSERT_EQUAL(xlog_writer_commit(w, "ccc", 4), 0);
    xlog_writer_close(w);

    /* Corrupt the size field of the second record (offset 12) */
    int fd = open("test.xlog", O_RDWR);
    CU_ASSERT_FATAL(fd >= 0);
    uint8_t bad_size[4] = { 0, 0, 0, 0 };
    lseek(fd, 12, SEEK_SET);
    write(fd, bad_size, 4);
    close(fd);

    /* Without flag: stops at bad size */
    char *rbuf;
    ssize_t sz;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 4);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "aaa");
    free(rbuf);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_ERR_SIZE);
    xlog_reader_close(r);

    /* With XLOG_SKIP_BADSIZE: scans forward, finds third record */
    r = xlog_reader_open_ex("test.xlog", UINT32_MAX, XLOG_SKIP_BADSIZE);
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 4);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "aaa");
    free(rbuf);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 4);
    CU_ASSERT_STRING_EQUAL_FATAL(rbuf, "ccc");
    free(rbuf);
    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);
    xlog_reader_close(r);
}

static void test_xlog_2writers(void) {
    unlink("test.xlog");

    xlog_writer_t *w1 = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w1);
    xlog_writer_t *w2 = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w2);

    for(int i = 0; i < 50000; i++) {
        char buf[32];
        sprintf(buf, "Hello, world! %d", 2*i);
        xlog_writer_commit(w1, buf, strlen(buf) + 1);
        sprintf(buf, "Hello, world! %d", 2*i + 1);
        xlog_writer_commit(w2, buf, strlen(buf) + 1);
    }
    xlog_writer_close(w1);
    xlog_writer_close(w2);

    char *rbuf;
    ssize_t sz;

    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    for(int i = 0; i < 100000; i++) {
        sz = xlog_reader_next(r, (void **)&rbuf);
        CU_ASSERT_EQUAL_FATAL(sz, (ssize_t)(strlen(rbuf) + 1));
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_STRING_EQUAL_FATAL(buf, rbuf);
        free(rbuf);
    }

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, XLOG_EOF);

    xlog_reader_close(r);
}

int main(void) {
    CU_pSuite suite = NULL;

    if(CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();

    suite = CU_add_suite("crc32c", NULL, NULL);
    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "crc32c", test_crc32c)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    suite = CU_add_suite("xlog", NULL, NULL);
    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_basic", test_xlog_basic)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_multi", test_xlog_multi)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_reopen_append", test_xlog_reopen_append)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_max_record_size", test_xlog_max_record_size)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_errors", test_xlog_errors)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_skip_corrupt", test_xlog_skip_corrupt)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_skip_badsize", test_xlog_skip_badsize)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "xlog_2writers", test_xlog_2writers)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
