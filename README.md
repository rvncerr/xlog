xlog
====

Binary append-only log library in C with CRC32C integrity checks.

- Atomic writes via `O_APPEND` + `writev` — concurrent writers, lock-free readers
- `fdatasync` after each commit by default (opt out with `XLOG_NOSYNC`)
- Little-endian on-disk format, portable across architectures
- Configurable max record size to guard against corruption
- Negative return codes for error reporting (`XLOG_ERR_IO`, `XLOG_ERR_CRC`, etc.)
- POSIX only (Linux, macOS, FreeBSD, etc.) — requires `writev`, `fdatasync`, `O_APPEND`

## Usage

```c
/* Write */
xlog_writer_t *w = xlog_writer_open("my.xlog");
xlog_writer_commit(w, data, len);
xlog_writer_close(w);

/* Read */
xlog_reader_t *r = xlog_reader_open("my.xlog");
void *buf;
ssize_t sz;
while ((sz = xlog_reader_next(r, &buf)) > 0) {
    /* process buf (sz bytes) */
    free(buf);
}
if (sz < 0) { /* handle error */ }
xlog_reader_close(r);
```

## Build

Requires [Conan](https://conan.io/) for the CUnit test dependency.

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(pwd)"
make
make && ./xlog_test
```

## API

See [include/xlog.h](include/xlog.h).
