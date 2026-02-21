#include "xlog.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

static void test_basic() {
    unlink("test_cpp.xlog");

    {
        xlog::writer w("test_cpp.xlog");
        w.commit("hello", 6);
        w.commit("world", 6);
    }

    {
        xlog::reader r("test_cpp.xlog");
        auto rec1 = r.next();
        assert(rec1.size() == 6);
        assert(memcmp(rec1.data(), "hello", 6) == 0);

        auto rec2 = r.next();
        assert(rec2.size() == 6);
        assert(memcmp(rec2.data(), "world", 6) == 0);

        auto eof = r.next();
        assert(eof.empty());
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
        auto rec = r.next();
        assert(rec.size() == sizeof(record));
        auto *p = reinterpret_cast<const record *>(rec.data());
        assert(p->x == 10 && p->y == 20);

        rec = r.next();
        p = reinterpret_cast<const record *>(rec.data());
        assert(p->x == 30 && p->y == 40);
    }

    unlink("test_cpp.xlog");
}

static void test_errors() {
    unlink("test_cpp.xlog");

    {
        xlog::writer w("test_cpp.xlog");
        try {
            w.commit("data", 0);
            assert(false);
        } catch(const xlog::error &e) {
            assert(e.code() == XLOG_ERR_SIZE);
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
    auto rec = r2.next();
    assert(rec.size() == 6);
    assert(memcmp(rec.data(), "moved", 6) == 0);

    unlink("test_cpp.xlog");
}

int main() {
    test_basic();
    test_struct();
    test_errors();
    test_move();
    printf("All C++ tests passed.\n");
    return 0;
}
