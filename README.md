xlog
====

Binary append-only log with CRC32C checksums. Supports concurrent writers/readers.

## Build

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(pwd)"
make
```

## API

See [xlog.h](xlog.h).
