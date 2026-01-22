# Global source for C++ project version - format must be maintained to avoid breaking regex
set(RELEASE_VERSION 3.9.24)

# Setup CMake and conan config
cmake_minimum_required(VERSION 3.15)
include(cmake/ThirdParty.cmake)

# Build Project
project(novatel_edie VERSION ${RELEASE_VERSION} LANGUAGES CXX)

# Disable compiler optimization known to cause incompatibility with certain packages (pandas + pyarrow v18)
# Background info on issue: https://stackoverflow.com/questions/78598141/first-stdmutexlock-crashes-in-application-built-with-latest-visual-studio
add_definitions(-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_PYTHON "Build Python bindings" OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE "Set -fPIC" ON)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Export compile commands" ON)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(COVERAGE "Coverage" OFF)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(GNUInstallDirs)
include(cmake/SetDefaultProfile.cmake)
include(cmake/CompilerOptions.cmake)
include(cmake/Utils.cmake)
# For custom Find*.cmake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

# Output all binaries in the same directory for easier testing
string(TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM)
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} COMPILER)
set(OUTPUT_DIR_BASE "${CMAKE_BINARY_DIR}/bin/${OPERATING_SYSTEM}-${CMAKE_SYSTEM_PROCESSOR}-${COMPILER}")

# For multi-config generators (Ninja Multi-Config, Visual Studio), set per-config output directories
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR_BASE}-$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR_BASE}-$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR_BASE}-$<CONFIG>")
set(OUTPUT_DIR "${OUTPUT_DIR_BASE}-$<CONFIG>")

# Look for shared libs in the same directory as the executable when running tests
set(CMAKE_BUILD_RPATH "\$ORIGIN")

# Put info into version header
execute_process(COMMAND ${CMAKE_COMMAND}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    -D DST=${CMAKE_CURRENT_SOURCE_DIR}/include/novatel_edie/version.h
    -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
    -D GIT_BRANCH=${GIT_BRANCH}
    -D RELEASE_VERSION=${RELEASE_VERSION}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)

# Build EDIE components
add_subdirectory(src/common)
add_subdirectory(src/decoders/common)
add_subdirectory(src/decoders/oem)

# Add an aggregate target for all components
add_library(novatel_edie INTERFACE)
add_library(novatel_edie::novatel_edie ALIAS novatel_edie)

if(NOT BUILD_SHARED_LIBS)
    # Use whole-archive linking with oem_decoder to ensure framer auto-registration is included
    # Only needed for static libraries - shared libraries export all symbols automatically
    if(MSVC)
        target_link_options(novatel_edie INTERFACE /WHOLEARCHIVE:$<TARGET_FILE:oem_decoder>)
    else()
        target_link_options(novatel_edie INTERFACE
            -Wl,--whole-archive
            $<TARGET_FILE:oem_decoder>
            -Wl,--no-whole-archive
            $<TARGET_FILE:common>
            $<TARGET_FILE:decoders_common>
        )
    endif()
endif()

target_link_libraries(novatel_edie INTERFACE
    oem_decoder
    common
    decoders_common
)

if(BUILD_PYTHON)
    add_subdirectory(python)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    # Enable CTest and generation of config files without cluttering targets
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21730
    set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
    include(CTest)
    add_subdirectory(src/common/test)
    add_subdirectory(src/decoders/common/test)
    add_subdirectory(src/decoders/oem/test)
endif()

if(BUILD_BENCHMARKS OR BUILD_EXAMPLES OR BUILD_TESTS)
    # Copy shared libs to the build output directory for tests and examples
    copy_third_party_shared_libs("${OUTPUT_DIR_BASE}")
    copy_cpp_runtime_dlls("${OUTPUT_DIR}")
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES database/database.json DESTINATION ${CMAKE_INSTALL_DATADIR}/novatel_edie)
install_novatel_edie_cmake_config()
