cmake_minimum_required(VERSION 3.14)
project(EDIE VERSION 1.0.0)

option(COVERAGE "Coverage" OFF)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_STANDARD 17)

include_directories(".")
include_directories("lib") # spdlog's include structure is relative to its root folder

include(FetchContent)

add_compile_definitions(STRICT)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

# Check platforms
if (CMAKE_HOST_WIN32)
   set(WINDOWS 1)
   message ("CMAKE Host: Windows")
elseif(CMAKE_HOST_UNIX)
   set(LINUX 1)
   message ("CMAKE Host: Linux")
endif()

message ("CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})
message ("CMAKE_LIB_SHARED: " ${CMAKE_LIB_SHARED})

if(NOT DEFINED ARCH)
   message("ARCH is not Set, default to x64")
   set(ARCH "x64")
endif()

if(NOT DEFINED GIT_BRANCH)
   set(GIT_BRANCH "")
endif()

if(WINDOWS)
   message ("ARCH is: " ${ARCH})
   set_property(GLOBAL PROPERTY USE_FOLDERS ON)
   add_definitions(-D_CRT_SECURE_NO_WARNINGS)
   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Zi /utf-8")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /utf-8")
   message ("cxx Flags: " ${CMAKE_CXX_FLAGS} /W4)
   message ("cxx Flags Debug: " ${CMAKE_CXX_FLAGS_DEBUG})
   message ("cxx Flags Release: " ${CMAKE_CXX_FLAGS_RELEASE})
   message ("CMAKE_GENERATOR_PLATFORM: " ${CMAKE_GENERATOR_PLATFORM})
   message ("CMAKE_GENERATOR_TOOLSET: " ${CMAKE_GENERATOR_TOOLSET})
endif()

if(LINUX)
    # if 32 bit then,
    # other way to check 32 bit if (CMAKE_CL_64) ?
    if((ARCH STREQUAL "x86") OR (${CMAKE_SYSTEM_PROCESSOR}  STREQUAL "i686"))
        message("ARCH is x86 & Building for 32 bit")
        set(CMAKE_C_FLAGS_DEBUG "-m32 -g")
        set(CMAKE_CXX_FLAGS_DEBUG "-m32 -g -fPIC -std=c++17")
        set(CMAKE_C_FLAGS_RELEASE "-m32 -O3 -DNDEBUG -s")
        set(CMAKE_CXX_FLAGS_RELEASE "-m32 -Wall -O3 -fPIC -D_LINUX_ -DNDEBUG -s -std=c++17")
    else()
        # set C&C++ flags for debug and release
        message("ARCH is x64 & Building for 64 bit")
        set(CMAKE_C_FLAGS_DEBUG "-g")
        set(CMAKE_CXX_FLAGS_DEBUG "-g -fPIC -D_LINUX_ -std=c++17")
        set(CMAKE_C_FLAGS_RELEASE "-s -O3 -DNDEBUG")
        set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O3 -fPIC -D_LINUX_ -DNDEBUG -s -std=c++17")
    endif()

    if(COVERAGE)
        message("Coverage is On")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} --coverage")
    endif()

   # Debug information
   message ("CMAKE_C_FLAGS_DEBUG:    " ${CMAKE_C_FLAGS_DEBUG})
   message ("CMAKE_CXX_FLAGS_DEBUG:  " ${CMAKE_CXX_FLAGS_DEBUG})
   message ("CMAKE_C_FLAGS_RELEASE:  " ${CMAKE_C_FLAGS_RELEASE})
   message ("CMAKE_CXX_FLAGS_RELEASE:" ${CMAKE_CXX_FLAGS_RELEASE})
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")

# Fetch the GoogleTest library
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# build EDIE components
add_subdirectory(src/decoders)
add_subdirectory(src/hw_interface)

# Find Git package, if not need to install manually or through .yml file
find_package(Git)
if(Git_FOUND)
  message("Git found: " ${GIT_EXECUTABLE})
endif()

# Build version of EDIE through cmake
if(GIT_EXECUTABLE)
    execute_process(COMMAND ${CMAKE_COMMAND}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        -D DST=${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D GIT_BRANCH=${GIT_BRANCH}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)
endif()

add_subdirectory(src/decoders/common/test)
add_subdirectory(src/decoders/novatel/test)
add_subdirectory(src/hw_interface/stream_interface/test)

if(WINDOWS)
    add_subdirectory(examples/novatel/command_encoding)
    add_subdirectory(examples/novatel/converter_fileparser)
    add_subdirectory(examples/novatel/converter_components)
    add_subdirectory(examples/novatel/converter_parser)
    add_subdirectory(examples/novatel/dynamic_fileparser)
    add_subdirectory(examples/novatel/dynamic_components)
    add_subdirectory(examples/novatel/dynamic_parser)
    add_subdirectory(examples/novatel/rangecmp)
    add_subdirectory(examples/novatel/rxconfig)
endif()

# This is an archive Static build(after combining stream, common, novatel component into libEDIE.a) in Linux
# if we do not require it , We can comment this section.
if(LINUX)
    add_custom_target(${PROJECT_NAME} ALL COMMAND ${CMAKE_AR} rc libEDIE.a $<TARGET_FILE:stream_interface> $<TARGET_FILE:common> $<TARGET_FILE:novatel>)
    install(FILES ${CMAKE_BINARY_DIR}/libEDIE.a DESTINATION lib)
endif()
