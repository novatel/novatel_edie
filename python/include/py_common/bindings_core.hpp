// All nanobind-related includes are collected here to minimize the risk of ODR issues.
// https://nanobind.readthedocs.io/en/latest/faq.html#compilation-fails-with-a-static-assertion-mentioning-nb-make-opaque
#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#if defined(__GNUC__) || defined(__clang__)
#define PYCOMMON_EXPORT __attribute__((visibility("default")))
#else
#define PYCOMMON_EXPORT
#endif
