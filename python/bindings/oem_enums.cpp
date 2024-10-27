#include "bindings_core.hpp"
#include "message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_oem_enums(nb::module_& m)
{
    nb::module_ enums_mod = m.def_submodule("enums", "");
    for (const auto& [_, enum_type] : MessageDbSingleton::get()->GetEnumsByIdMap())
    {
        nb::object name = enum_type.attr("_name");
        enums_mod.attr(name) = enum_type;
    }
}
