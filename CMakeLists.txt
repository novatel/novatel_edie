cmake_minimum_required(VERSION 3.15)
include(cmake/third_party.cmake)

project(novatel_edie VERSION 3.3.7 LANGUAGES CXX)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE "Set -fPIC" ON)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(COVERAGE "Coverage" OFF)

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
    message(WARNING "Unable to identify OS. Update script to support distribution or OS")
endif()

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        # shared spdlog requires MultiThreadedDLL
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
    add_definitions(-DWIN32 -D_WINDOWS)
    add_compile_options(/W4 /GR /EHsc /utf-8 /wd4244 /wd4996 /wd4127)
    add_compile_options("$<$<CONFIG:Release>:/Ox;/Ob2>")
    if(WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    add_compile_options(-Wall -Wextra -pedantic -Wold-style-cast)
    add_compile_options("$<$<CONFIG:Release>:-O3>")

    if(WINDOWS)
        # TODO: we shouldn't have to do this, something is bloating an object file
        add_compile_options($<$<CONFIG:Debug>:-O1>)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wno-format-truncation -Wno-restrict)
    else()
        add_compile_options(-Wno-unknown-pragmas -Wno-deprecated-declarations)
    endif()

    if(WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()

    if(COVERAGE)
        message("Coverage is On")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
else()
    message(WARNING "Unable to identify compiler.")
endif()

# Output all binaries in the same directory for easier testing
foreach(config ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} BUILD_TYPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/bin/${config}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/bin/${config}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${CMAKE_BINARY_DIR}/bin/${config}")
endforeach()

# Find Git package, if not need to install manually or through .yml file
find_package(Git)
if(NOT Git_FOUND)
    message(FATAL_ERROR "Git was not found. Install Git and make sure it is in your PATH.")
endif()

if(NOT DEFINED GIT_BRANCH)
    set(GIT_BRANCH "main")
endif()

# Build version of EDIE through cmake
if(GIT_EXECUTABLE)
    execute_process(COMMAND ${CMAKE_COMMAND}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        -D DST=${CMAKE_CURRENT_SOURCE_DIR}/src/common/include/novatel_edie/version.h
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D GIT_BRANCH=${GIT_BRANCH}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)
endif()

# Build EDIE components
add_subdirectory(src)

add_library(novatel_edie INTERFACE)
add_library(novatel_edie::novatel_edie ALIAS novatel_edie)
target_link_libraries(novatel_edie INTERFACE
    novatel_edie::common
    novatel_edie::decoders
    novatel_edie::stream_interface
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/common/test)
    add_subdirectory(src/decoders/test)
    add_subdirectory(src/stream_interface/test)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(WIN32)
    # Copy third-party DLLs to the build directory for tests
    copy_third_party_dlls()
    # Copy C++ runtime DLL for non-MSVC compilers
    if(NOT MSVC)
        if(NOT EXISTS CMAKE_CXX_COMPILER)
            find_program(CXX_COMPILER_PATH NAMES "${CMAKE_CXX_COMPILER}")
        else()
            set(CXX_COMPILER_PATH "${CMAKE_CXX_COMPILER}")
        endif()
        get_filename_component(COMPILER_BIN_DIR "${CXX_COMPILER_PATH}" DIRECTORY)
        foreach(stdcpp_library libstdc++-6.dll libc++.dll)
            if(EXISTS "${COMPILER_BIN_DIR}/${stdcpp_library}")
                foreach(config ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
                    file(COPY "${COMPILER_BIN_DIR}/${stdcpp_library}" DESTINATION "${CMAKE_BINARY_DIR}/bin/${config}")
                endforeach()
            endif()
        endforeach()
    endif()
endif()
