#include "py_common/bindings_core.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

void py_oem::init_novatel_oem_enums(nb::module_& m)
{
    for (const auto& [name, enum_type] : py_oem::MessageDbSingleton::get()->GetEnumsByNameDict()) { m.attr(name.c_str()) = enum_type; }
}

void py_oem::init_novatel_oem_messages(nb::module_& m)
{
    for (const auto& [name, message_type_struct] : py_oem::MessageDbSingleton::get()->GetMessagesByNameDict())
    {
        m.attr(name.c_str()) = message_type_struct.python_type;
    }
    for (const auto& [def, field_type] : py_oem::MessageDbSingleton::get()->GetFieldsByDefDict())
    {
        m.attr(field_type.attr("__class__").attr("__name__")) = field_type;
    }
}

void py_oem::init_message_db_singleton(nb::module_& m)
{
    m.def("get_builtin_database", &py_oem::MessageDbSingleton::get, "Get the JSON database built-in to the package.");
}
