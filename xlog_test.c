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

static void test_xlog(void) {
    unlink("test.xlog");

    xlog_writer_t *w = xlog_writer_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(w);

    xlog_reader_t *r = xlog_reader_open("test.xlog");
    CU_ASSERT_PTR_NOT_NULL_FATAL(r);

    char *buf = "Hello, world!";
    xlog_writer_commit(w, buf, strlen(buf) + 1);

    char *buf2 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                 "sed do eiusmod tempor incididunt ut labore et dolore magna "
                 "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
                 "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
                 "Duis aute irure dolor in reprehenderit in voluptate velit "
                 "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
                 "occaecat cupidatat non proident, sunt in culpa qui officia "
                 "deserunt mollit anim id est laborum.";
    xlog_writer_commit(w, buf2, strlen(buf2) + 1);
    xlog_writer_close(w);

    char *buf3;
    size_t sz = xlog_reader_next(r, (void **)&buf3);
    CU_ASSERT_EQUAL_FATAL(sz, strlen(buf) + 1);
    CU_ASSERT_STRING_EQUAL_FATAL(buf, buf3);
    free(buf3);

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

    if(NULL == CU_add_test(suite, "xlog", test_xlog)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}