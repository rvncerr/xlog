xlog
====

Binary append-only log with CRC32C checksums. Supports concurrent writers and lock-free readers.

- Little-endian on-disk format, portable across architectures
- `fdatasync` after each commit by default (opt out with `XLOG_NOSYNC`)
- Configurable max record size to guard against corruption
- POSIX `fcntl` locking for writer synchronization

## Usage

```c
xlog_writer_t *w = xlog_writer_open("my.xlog");
xlog_writer_commit(w, data, len);
xlog_writer_close(w);

xlog_reader_t *r = xlog_reader_open("my.xlog");
void *buf;
ssize_t sz;
while ((sz = xlog_reader_next(r, &buf)) > 0) {
    /* process buf (sz bytes), then free */
    free(buf);
}
xlog_reader_close(r);
```

## Build

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(pwd)"
make
```

## API

See [include/xlog.h](include/xlog.h).
