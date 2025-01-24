#include "bindings_core.hpp"
#include "message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_oem_enums(nb::module_& m)
{
    for (const auto& [name, enum_type] : MessageDbSingleton::get()->GetEnumsByNameDict()) //
    {
        m.attr(name.c_str()) = enum_type;
    }
}

void init_novatel_oem_messages(nb::module_& m)
{
    for (const auto& [name, message_type] : MessageDbSingleton::get()->GetMessagesByNameDict()) { 
        m.attr((name + "MessageBody").c_str()) = message_type; 
    }
}
