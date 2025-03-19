#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_oem_enums(nb::module_& m)
{
    for (const auto& [name, enum_type] : MessageDbSingleton::get()->GetEnumsByNameDict()) { m.attr(name.c_str()) = enum_type; }
}

void init_novatel_oem_messages(nb::module_& m)
{
    for (const auto& [name, message_type_struct] : MessageDbSingleton::get()->GetMessagesByNameDict())
    {
        m.attr(name.c_str()) = message_type_struct->python_type;
    }
    for (const auto& [name, field_type] : MessageDbSingleton::get()->GetFieldsByNameDict()) { m.attr(name.c_str()) = field_type; }
}
