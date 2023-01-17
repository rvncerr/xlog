#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "xlog.h"

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