#include "xlog.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// This suite used assert(), which the documented Release build compiles out
// via -DNDEBUG — every check vanished and the binary passed without testing
// anything. These checks are ordinary code, so they run under any build type,
// and a failure reaches the exit code instead of being silently dropped.
static int failures = 0;

static void check_failed(const char *what, const char *file, int line) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, what);
    ++failures;
}

#define CHECK(cond) ((cond) ? (void)0 : check_failed(#cond, __FILE__, __LINE__))

// For a condition whose failure would make the rest of the test unsafe or
// meaningless — reading a record that came back the wrong size, say.
#define CHECK_FATAL(cond)                                     \
    do {                                                      \
        if(!(cond)) {                                         \
            check_failed(#cond, __FILE__, __LINE__);          \
            return;                                           \
        }                                                     \
    } while(0)

// Reached only when something that had to throw did not.
#define CHECK_FAILED(msg) check_failed(msg, __FILE__, __LINE__)

static void test_basic() {
    unlink("test_cpp.xlog");

    {
        xlog::writer w("test_cpp.xlog");
        w.commit("hello", 6);
        w.commit("world", 6);
    }

    {
        xlog::reader r("test_cpp.xlog");
        auto rec1 = r.next(4096);
        CHECK_FATAL(rec1.size() == 6);
        CHECK(memcmp(rec1.data(), "hello", 6) == 0);

        auto rec2 = r.next(4096);
        CHECK_FATAL(rec2.size() == 6);
        CHECK(memcmp(rec2.data(), "world", 6) == 0);

        auto eof = r.next(4096);
        CHECK(eof.empty());
    }

    unlink("test_cpp.xlog");
}

static void test_struct() {
    unlink("test_cpp.xlog");

    struct record { int32_t x; int32_t y; };

    {
        xlog::writer w("test_cpp.xlog");
        w.commit(record{10, 20});
        w.commit(record{30, 40});
    }

    {
        xlog::reader r("test_cpp.xlog");
        auto rec = r.next(4096);
        CHECK_FATAL(rec.size() == sizeof(record));
        auto *p = reinterpret_cast<const record *>(rec.data());
        CHECK(p->x == 10 && p->y == 20);

        rec = r.next(4096);
        CHECK_FATAL(rec.size() == sizeof(record));
        p = reinterpret_cast<const record *>(rec.data());
        CHECK(p->x == 30 && p->y == 40);
    }

    unlink("test_cpp.xlog");
}

static void test_errors() {
    unlink("test_cpp.xlog");

    {
        xlog::writer w("test_cpp.xlog");
        try {
            w.commit("data", 0);
            CHECK_FAILED("commit of a zero-length record did not throw");
        } catch(const xlog::error &e) {
            CHECK(e.code() == XLOG_ERR_SIZE);
        }
    }

    unlink("test_cpp.xlog");
}

static void test_move() {
    unlink("test_cpp.xlog");

    xlog::writer w1("test_cpp.xlog");
    xlog::writer w2 = std::move(w1);
    w2.commit("moved", 6);

    xlog::reader r1("test_cpp.xlog");
    xlog::reader r2 = std::move(r1);
    auto rec = r2.next(4096);
    CHECK_FATAL(rec.size() == 6);
    CHECK(memcmp(rec.data(), "moved", 6) == 0);

    unlink("test_cpp.xlog");
}

// Open failures must carry the errno text; XLOG_ERR_IO alone says nothing
// about why the open failed.
static void test_open_error() {
    const char *missing = "no_such_dir_xlog/test_cpp.xlog";

    try {
        xlog::writer w(missing);
        CHECK_FAILED("opening a writer under a missing directory did not throw");
    } catch(const xlog::error &e) {
        CHECK(e.code() == XLOG_ERR_IO);
        CHECK(strstr(e.what(), missing) != nullptr);
        CHECK(strstr(e.what(), strerror(ENOENT)) != nullptr);
    }

    try {
        xlog::reader r(missing);
        CHECK_FAILED("opening a reader on a missing log did not throw");
    } catch(const xlog::error &e) {
        CHECK(e.code() == XLOG_ERR_IO);
        CHECK(strstr(e.what(), missing) != nullptr);
        CHECK(strstr(e.what(), strerror(ENOENT)) != nullptr);
    }

    // The _ex opens report their own EINVAL, not a filesystem error.
    try {
        xlog::writer w("test_cpp.xlog", 0);
        CHECK_FAILED("max_record_size of 0 did not throw");
    } catch(const xlog::error &e) {
        CHECK(strstr(e.what(), strerror(EINVAL)) != nullptr);
    }
    unlink("test_cpp.xlog");
}

int main() {
    test_basic();
    test_struct();
    test_errors();
    test_move();
    test_open_error();

    if(failures) {
        fprintf(stderr, "%d C++ check(s) failed.\n", failures);
        return EXIT_FAILURE;
    }
    printf("All C++ tests passed.\n");
    return EXIT_SUCCESS;
}
