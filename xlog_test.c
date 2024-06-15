#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

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
    xlog_writer_commit(w, wbuf, strlen(wbuf) + 1);
    xlog_writer_close(w);

    char *rbuf;
    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    size_t sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, strlen(rbuf) + 1);
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
        xlog_writer_commit(w, buf, strlen(buf) + 1);
    }
    xlog_writer_close(w);

    char *rbuf;
    size_t sz;

    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    for(int i = 0; i < 1000; i++) {
        sz = xlog_reader_next(r, (void **)&rbuf);
        CU_ASSERT_EQUAL_FATAL(sz, strlen(rbuf) + 1);
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_STRING_EQUAL_FATAL(buf, rbuf);
        free(rbuf);
    }

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 0);

    xlog_reader_reset(r);

    for(int i = 0; i < 1000; i++) {
        sz = xlog_reader_next(r, (void **)&rbuf);
        CU_ASSERT_EQUAL_FATAL(sz, strlen(rbuf) + 1);
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_STRING_EQUAL_FATAL(buf, rbuf);
        free(rbuf);
    }

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 0);

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
    size_t sz;

    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);
    for(int i = 0; i < 100000; i++) {
        sz = xlog_reader_next(r, (void **)&rbuf);
        CU_ASSERT_EQUAL_FATAL(sz, strlen(rbuf) + 1);
        char buf[32];
        sprintf(buf, "Hello, world! %d", i);
        CU_ASSERT_STRING_EQUAL_FATAL(buf, rbuf);
        free(rbuf);
    }

    sz = xlog_reader_next(r, (void **)&rbuf);
    CU_ASSERT_EQUAL_FATAL(sz, 0);

    xlog_reader_close(r);
}

int main(void) {
    CU_pSuite suite = NULL;

    if(CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();

    suite = CU_add_suite("crc32r", NULL, NULL);
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

    if(NULL == CU_add_test(suite, "xlog_2writers", test_xlog_2writers)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
