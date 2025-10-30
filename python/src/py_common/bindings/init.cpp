#include "py_common/init_bindings_functions.hpp"

#include <memory>

#include <nanobind/nanobind.h>

#include "novatel_edie/common/logger.hpp"

namespace nb = nanobind;
using namespace novatel::edie::py_common;

NB_MODULE(python_common, m)
{
    nb::module_ internal_mod = m.def_submodule("_internal", "NOT PART OF THE PUBLIC API.");

    init_common(m);
    init_common_logger(m, internal_mod);
    init_logger_tester(internal_mod);
    init_exceptions(m);
    init_common_message_database(m);
    init_message_db_singleton(m);
}
