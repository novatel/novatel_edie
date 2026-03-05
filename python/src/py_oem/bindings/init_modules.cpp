#include "py_common/bindings_core.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

void py_oem::init_message_db_singleton(nb::module_& m, nb::module_& messagesMod, nb::module_& enumsMod)
{
    m.def("get_builtin_database", &py_oem::MessageDbSingleton::get, "Get the JSON database built-in to the package.");
    py_oem::MessageDbSingleton::get()->bindToModule(messagesMod, enumsMod);
}
