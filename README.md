xlog
====

Binary append-only log library in C with CRC32C integrity checks and C++ bindings.

- Atomic writes via `O_APPEND` + `writev` — concurrent writers, lock-free readers
- `fdatasync` after each commit by default (opt out with `XLOG_NOSYNC`)
- Little-endian on-disk format, portable across architectures
- Configurable max record size to guard against corruption
- Negative return codes for error reporting (`XLOG_ERR_IO`, `XLOG_ERR_CRC`, etc.)
- POSIX only (Linux, macOS, FreeBSD, etc.) — requires `writev`, `fdatasync`, `O_APPEND`

## Usage (C)

```c
#include "xlog.h"

/* Write */
xlog_writer *w = xlog_writer_open("my.xlog");
xlog_writer_commit(w, data, len);
xlog_writer_close(w);

/* Read */
xlog_reader *r = xlog_reader_open("my.xlog");
char buf[4096];
ssize_t sz;
while ((sz = xlog_reader_next(r, buf, sizeof(buf))) > 0) {
    /* process buf (sz bytes) */
}
if (sz < 0) { /* handle error */ }
xlog_reader_close(r);
```

## Usage (C++)

```cpp
#include "xlog.hpp"

/* Write — closes automatically */
{
    xlog::writer w("my.xlog");
    w.commit(data, len);
    w.commit(my_struct);  // any POD type
}

/* Read — returns std::vector<uint8_t>, throws on error */
{
    xlog::reader r("my.xlog");
    for (auto rec = r.next(); !rec.empty(); rec = r.next()) {
        /* process rec.data() / rec.size() */
    }
}
```

## Build

Requires [Conan](https://conan.io/) for the CUnit test dependency.

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(pwd)"
make
./test_xlog && ./test_xlog_cpp
```

## Thread Safety

- Multiple writers to the same file from separate handles are safe (`O_APPEND` + `writev` atomicity).
- A single `xlog_reader` or `xlog_writer` must not be used from multiple threads concurrently.
- Separate reader/writer handles may be used concurrently without synchronization.

## API

- C: [include/xlog.h](include/xlog.h)
- C++: [include/xlog.hpp](include/xlog.hpp)
