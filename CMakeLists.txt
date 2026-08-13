cmake_minimum_required(VERSION 3.10...3.31)
project(xlog VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CheckCCompilerFlag)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tests are built only for a top-level build: consumers embedding xlog with
# add_subdirectory() should not need CUnit.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(xlog_default_testing ON)
else()
    set(xlog_default_testing OFF)
endif()

option(BUILD_TESTING "Build the xlog test suites (requires CUnit)" ${xlog_default_testing})
option(XLOG_WERROR "Treat compiler warnings as errors" OFF)
option(XLOG_HW_CRC32C "Use hardware CRC32C instructions where available" ON)

set(xlog_warnings -Wall -Wextra -Wpedantic)
if(XLOG_WERROR)
    list(APPEND xlog_warnings -Werror)
endif()

# Hardware CRC32C is selected at compile time, so the resulting binary
# requires a CPU with those instructions: SSE4.2 (Intel Nehalem, 2008) or
# ARMv8 CRC. Distributors shipping binaries for older x86 hardware should
# configure with -DXLOG_HW_CRC32C=OFF to get the portable table version.
set(xlog_crc_flags "")
if(XLOG_HW_CRC32C)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" xlog_arch)
    if(xlog_arch MATCHES "x86_64|amd64|i[3-6]86")
        check_c_compiler_flag(-msse4.2 XLOG_HAVE_SSE42_FLAG)
        if(XLOG_HAVE_SSE42_FLAG)
            set(xlog_crc_flags "-msse4.2")
        endif()
    elseif(xlog_arch MATCHES "aarch64|arm64")
        check_c_compiler_flag("-march=armv8-a+crc" XLOG_HAVE_ARM_CRC_FLAG)
        if(XLOG_HAVE_ARM_CRC_FLAG)
            set(xlog_crc_flags "-march=armv8-a+crc")
        endif()
    endif()
endif()
if(xlog_crc_flags)
    set_source_files_properties(src/crc32c.c PROPERTIES COMPILE_FLAGS "${xlog_crc_flags}")
endif()

add_library(xlog SHARED src/xlog.c src/crc32c.c)
add_library(xlog::xlog ALIAS xlog)
set_target_properties(xlog PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_VISIBILITY_PRESET hidden)
target_compile_definitions(xlog PRIVATE XLOG_BUILDING)
target_compile_options(xlog PRIVATE ${xlog_warnings})
target_include_directories(xlog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(xlog PRIVATE src)

add_executable(xlog_dump src/xlog_dump.c)
target_link_libraries(xlog_dump PRIVATE xlog)
target_compile_options(xlog_dump PRIVATE ${xlog_warnings})

if(BUILD_TESTING)
    enable_testing()
    find_package(cunit REQUIRED)

    # crc32c is hidden in the shared library, so the C tests compile it in.
    add_executable(test_xlog tests/test_xlog.c src/crc32c.c)
    target_link_libraries(test_xlog PRIVATE xlog cunit::cunit)
    target_include_directories(test_xlog PRIVATE src)
    target_compile_options(test_xlog PRIVATE ${xlog_warnings})
    add_test(NAME xlog_c COMMAND test_xlog)

    add_executable(test_xlog_cpp tests/test_xlog.cpp)
    target_link_libraries(test_xlog_cpp PRIVATE xlog)
    target_compile_options(test_xlog_cpp PRIVATE ${xlog_warnings})
    add_test(NAME xlog_cpp COMMAND test_xlog_cpp)
endif()

install(TARGETS xlog
    EXPORT xlogTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS xlog_dump RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES include/xlog.h include/xlog.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT xlogTargets
    FILE xlogTargets.cmake
    NAMESPACE xlog::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/xlogConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/xlogConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/xlogConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/xlogConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/xlogConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)
