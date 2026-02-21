cmake_minimum_required(VERSION 3.10)
project(xlog C)

set(CMAKE_C_STANDARD 11)

find_package(cunit REQUIRED)

include_directories(include)

add_library(xlog SHARED src/xlog.c src/crc32c.c src/endian.c)

add_executable(xlog_dump src/xlog_dump.c src/hexdump.c)
target_link_libraries(xlog_dump xlog)

add_executable(xlog_test tests/xlog_test.c)
target_link_libraries(xlog_test xlog cunit::cunit)
