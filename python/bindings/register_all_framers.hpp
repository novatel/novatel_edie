#pragma once
#include <nanobind/nanobind.h>

#include "novatel_edie/decoders/common/framer_registration.hpp"

namespace nb = nanobind;

void init_register_all_framers(nb::module_& m);
