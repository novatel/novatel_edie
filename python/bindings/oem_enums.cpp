#include "bindings_core.hpp"
#include "message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_oem_enums(nb::module_& m)
{
    nb::module_ enums_mod = m.def_submodule("enums", "Enumerations used by NovAtel OEM message fields.");
    for (const auto& [name, enum_type] : MessageDbSingleton::get()->GetEnumsByNameDict()) //
    {
        enums_mod.attr(name.c_str()) = enum_type;
    }
}
