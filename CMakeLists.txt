cmake_minimum_required(VERSION 3.14)
project(novatel-edie VERSION 3.3.7)

enable_testing()

option(COVERAGE "Coverage" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_STANDARD 20)
set(ARCH "x64")

include(FetchContent)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Check platforms
if(CMAKE_HOST_WIN32)
    set(WINDOWS 1)
    cmake_host_system_information(RESULT OS_NAME QUERY OS_NAME)
    set(DISTRIB_NAME ${OS_NAME})
elseif(CMAKE_HOST_UNIX)
    set(LINUX 1)
    cmake_host_system_information(RESULT DISTRIB_NAME QUERY DISTRIB_NAME)
    cmake_host_system_information(RESULT DISTRIB_VERSION_ID QUERY DISTRIB_VERSION_ID)
    set(DISTRIB_NAME "${DISTRIB_NAME}-${DISTRIB_VERSION_ID}")
else()
    message(FATAL_ERROR "Unable to identify OS. Update script to support distribution or OS")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "/WX /W4 /DWIN32 /D_WINDOWS /GR /EHsc /utf-8 /wd4244 /wd4996 /wd4127")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1 /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /Ox /Ob2 /DNDEBUG")

elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-Wall -Werror -fPIC -Wfatal-errors -Wold-style-cast -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    add_link_options("$<$<CONFIG:Release>:-s>")

    if(WINDOWS)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1") # TODO: we shouldn't have to do this, something is bloating an object file
    endif()
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-truncation -Wno-error=format -Wno-format -Wno-format-extra-args -Wno-error=restrict -Wno-restrict")
    endif()
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wno-unqualified-std-cast-call -Wno-unknown-pragmas")
    endif()
else()
    message(FATAL_ERROR "Unable to identify compiler.")
endif()

if(COVERAGE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
endif()

# Output all binaries in the same directory for easier testing
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}>)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")

# Fetch the GoogleTest library
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

# Create src/version.h based on CMake project and git versions
execute_process(COMMAND ${CMAKE_COMMAND}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    -D DST=${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
    -D PROJECT_VERSION=${PROJECT_VERSION}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# build EDIE components
add_subdirectory(examples)
add_subdirectory(src/decoders)
add_subdirectory(src/hw_interface)
add_subdirectory(src/decoders/common/test)
add_subdirectory(src/decoders/novatel/test)
add_subdirectory(src/hw_interface/stream_interface/test)
