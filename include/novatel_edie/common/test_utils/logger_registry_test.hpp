//-----------------------------------------------------------------------
//! \file logger_registry_test.hpp
//! \brief Helper for skipping logger-registry tests when spdlog is statically
//!        linked into a shared library.
//!
//! In Python builds (BUILD_PYTHON=ON) the EDIE libraries are built as shared
//! libraries with spdlog linked statically into them. spdlog's global registry
//! lives in static storage, so the copy inside the EDIE shared library is a
//! separate instance from the one in the test executable. As a result, registry
//! assertions in the *.LOGGER test cases (e.g. spdlog::get("novatel_framer"))
//! cannot see loggers registered on the library side and fail.
//!
//! These assertions describe behavior that is only valid when a single spdlog
//! registry is shared across the test executable and the code under test, so the
//! affected tests are skipped in that configuration only. The macro is a no-op in
//! every other build, leaving the tests active.
//!
//! NOVATEL_EDIE_STATIC_SPDLOG is defined on the test targets by CMake when
//! BUILD_PYTHON is ON (see the BUILD_TESTS block in the top-level CMakeLists.txt).
//-----------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>

#ifdef NOVATEL_EDIE_STATIC_SPDLOG
#define SKIP_IF_STATIC_SPDLOG_REGISTRY()                                                                                                             \
    GTEST_SKIP() << "spdlog is statically linked into the shared library (Python build); its global registry is not shared with the test "           \
                    "executable, so logger-registry assertions are not applicable."
#else
#define SKIP_IF_STATIC_SPDLOG_REGISTRY() ((void)0)
#endif
