#include "py_common/message_db_singleton.hpp"

#include "novatel_edie/decoders/common/json_db_reader.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/message_database.hpp"
#include "py_common/init_bindings.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_common::init_message_db_singleton(nb::module_& m)
{
    m.def("get_builtin_database", &py_common::MessageDbSingleton::get, "Get the JSON database built-in to the package.");
}
