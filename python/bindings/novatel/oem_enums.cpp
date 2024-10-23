#include "bindings_core.hpp"
#include "json_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

const std::unordered_map<std::string, nb::object>& JsonDbSingleton::getEnumsByIdMap()
{
    static std::unordered_map<std::string, nb::object> enums_by_id{};
    if (enums_by_id.empty())
    {
        nb::object IntEnum = nb::module_::import_("enum").attr("IntEnum");
        enums_by_id.clear();
        for (const auto& enum_def : get()->EnumDefinitions())
        {
            nb::dict values;
            const char* enum_name = enum_def->name.c_str();
            for (const auto& enumerator : enum_def->enumerators) { values[enumerator.name.c_str()] = enumerator.value; }
            nb::object enum_type = IntEnum(enum_name, values);
            enum_type.attr("_name") = enum_name;
            enum_type.attr("_id") = enum_def->_id;
            enums_by_id[enum_def->_id.c_str()] = enum_type;
        }
    }
    return enums_by_id;
}

void init_novatel_oem_enums(nb::module_& m)
{
    nb::module_ message_enums_mod = m.def_submodule("message_enums", "");
    for (const auto& [_, enum_type] : JsonDbSingleton::getEnumsByIdMap()) //
    {
        nb::object name = enum_type.attr("_name");
        message_enums_mod.attr(name) = enum_type;
    }
}
