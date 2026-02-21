cmake_minimum_required(VERSION 3.10)
project(xlog C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

find_package(cunit REQUIRED)

add_library(xlog SHARED src/xlog.c src/crc32c.c)
target_include_directories(xlog PUBLIC include)
target_include_directories(xlog PRIVATE src)

add_executable(xlog_dump src/xlog_dump.c)
target_link_libraries(xlog_dump xlog)

add_executable(test_xlog tests/test_xlog.c)
target_link_libraries(test_xlog xlog cunit::cunit)
target_include_directories(test_xlog PRIVATE src)

add_executable(test_xlog_cpp tests/test_xlog.cpp)
target_link_libraries(test_xlog_cpp xlog)
