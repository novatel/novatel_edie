#pragma once

#include "py_common/bindings_core.hpp"


namespace nb = nanobind;

namespace novatel::edie::py_common {

void init_common_logger(nb::module_&, nb::module_&);
void init_logger_tester(nb::module_&);


} // namespace novatel::edie::py_common
