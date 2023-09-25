// All nanobind-related includes are collected here to minimize the risk of ODR issues.
// https://nanobind.readthedocs.io/en/latest/faq.html#compilation-fails-with-a-static-assertion-mentioning-nb-make-opaque
#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

#include "u32string.h"
