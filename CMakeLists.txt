cmake_minimum_required(VERSION 3.10)
project(xlog C)

set(CMAKE_C_STANDARD 11)

add_library(xlog SHARED xlog.c xlog.h crc32c.h crc32c.c)

add_executable(xlog_dump xlog_dump.c hexdump.c xlog)
target_link_libraries(xlog_dump xlog)

add_executable(xlog_test xlog_test.c xlog)
target_link_libraries(xlog_test xlog cunit)