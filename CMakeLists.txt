# Global source for C++ project version - format must be maintained to avoid breaking regex
set(RELEASE_VERSION 3.5.5)

# Setup CMake and conan config
cmake_minimum_required(VERSION 3.15)
include(cmake/ThirdParty.cmake)

# Build Project
project(novatel_edie VERSION ${RELEASE_VERSION} LANGUAGES CXX)


option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_PYTHON "Build Python bindings" OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE "Set -fPIC" ON)
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
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
string(TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM)
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} COMPILER)
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/${OPERATING_SYSTEM}-${CMAKE_SYSTEM_PROCESSOR}-${COMPILER}-${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${BUILD_TYPE} "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${BUILD_TYPE} "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${BUILD_TYPE} "${OUTPUT_DIR}")

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
target_link_libraries(novatel_edie INTERFACE
    common
    decoders_common
    oem_decoder
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
    enable_testing()
    add_subdirectory(src/common/test)
    add_subdirectory(src/decoders/common/test)
    add_subdirectory(src/decoders/oem/test)
endif()

if(BUILD_BENCHMARKS OR BUILD_EXAMPLES OR BUILD_TESTS)
    # Copy shared libs to the build output directory for tests and examples
    copy_cpp_runtime_dlls("${OUTPUT_DIR}")
    copy_third_party_shared_libs("${OUTPUT_DIR}")
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES database/database.json DESTINATION ${CMAKE_INSTALL_DATADIR}/novatel_edie)
install_novatel_edie_cmake_config()
