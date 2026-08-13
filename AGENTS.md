# Agent Instructions

## Rules

- **Never stage, commit, or push without an explicit command from the user.**
- Do not amend, rebase, or force-push existing commits unless explicitly asked.

## Project

xlog is a POSIX append-only binary log library. The core is written in
conventional C (C11); C++ bindings are a thin header-only wrapper (`xlog.hpp`).

- Public C API: `include/xlog.h`
- Public C++ API: `include/xlog.hpp` (header-only, wraps the C API)
- Internal headers (`src/crc32c.h`, `src/endian.h`) must not leak into the public ABI.

## Build

Requires CMake ≥ 3.10. Conan 2.x is needed only for the tests (CUnit).

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
         -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(pwd)"
cmake --build .
```

The library alone needs no dependencies: `cmake -B build -DBUILD_TESTING=OFF`.

Options: `BUILD_TESTING` (default ON at top level), `XLOG_WERROR` (default
OFF), `XLOG_HW_CRC32C` (default ON — set OFF for binaries that must run on
pre-SSE4.2 x86).

## Tests

Run both suites after any change, via ctest or directly:

```bash
cd build && ctest --output-on-failure
```

## Style

- C code: C11, no compiler-specific extensions in headers.
- Keep the C API self-contained — no C++ types, no STL.
- Symbols not part of the public API must use hidden visibility (enforced via
  `C_VISIBILITY_PRESET hidden` + `XLOG_API` macro for exports).
