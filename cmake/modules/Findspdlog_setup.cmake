# Findspdlog_setup.cmake
#
# This module tries to locate the spdlog_setup library using find_package(spdlog_setup CONFIG).
# If not found, it falls back to downloading and configuring it from an official release.

find_package(spdlog_setup CONFIG QUIET)

if(NOT spdlog_setup_FOUND)
    set(spdlog_setup_VERSION "1.1.0")
    set(spdlog_setup_URL "https://github.com/gegles/spdlog_setup/archive/refs/tags/v${spdlog_setup_VERSION}.tar.gz")
    set(spdlog_setup_SHA256 "80a37463a1cd2735f6f7af0b0dfb01a1ecc0a271b33fb29966564b9758f7c309")
    set(spdlog_setup_SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/spdlog_setup")
    set(spdlog_setup_INCLUDE_DIR "${spdlog_setup_SOURCE_DIR}/include")

    find_package(cpptoml REQUIRED CONFIG)

    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    include(FetchContent)
    FetchContent_Declare(
        spdlog_setup
        URL "${spdlog_setup_URL}"
        URL_HASH SHA256=${spdlog_setup_SHA256}
        SOURCE_DIR "${spdlog_setup_SOURCE_DIR}"
        EXCLUDE_FROM_ALL
    )
    FetchContent_GetProperties(spdlog_setup)
    if(NOT EXISTS "${spdlog_setup_INCLUDE_DIR}")
        message(STATUS "spdlog_setup not found. Fetching it from ${spdlog_setup_URL} instead...")
        FetchContent_MakeAvailable(spdlog_setup)
    endif()

    add_library(spdlog_setup::spdlog_setup INTERFACE IMPORTED)
    target_include_directories(spdlog_setup::spdlog_setup INTERFACE
        "$<BUILD_INTERFACE:${spdlog_setup_INCLUDE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/novatel_edie/third_party>"
    )
    target_link_libraries(spdlog_setup::spdlog_setup INTERFACE cpptoml)

    install(DIRECTORY "${spdlog_setup_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/novatel_edie/third_party
    )

    set(spdlog_setup_FOUND TRUE CACHE INTERNAL "")
    set(spdlog_setup_VENDORED TRUE CACHE INTERNAL "")
else()
    set(spdlog_setup_VENDORED FALSE CACHE INTERNAL "")
endif()
