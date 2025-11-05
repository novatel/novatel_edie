#pragma once

#include "py_common/bindings_core.hpp"


namespace nb = nanobind;

namespace novatel::edie::py_common {

void init_common(nb::module_& m);
void init_common_logger(nb::module_&, nb::module_&);
void init_logger_tester(nb::module_&);
void init_exceptions(nb::module_&);
void init_common_message_database(nb::module_&);
void init_message_db_singleton(nb::module_&);
void init_field_objects(nb::module_&);
void init_raw_data_classes(nb::module_&);


} // namespace novatel::edie::py_common
