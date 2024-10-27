#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"

namespace nb = nanobind;

namespace novatel::edie {

class PyMessageDatabase final : public MessageDatabase
{
  public:
    PyMessageDatabase() { UpdatePythonEnums(); }

    PyMessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_)
        : MessageDatabase(std::move(vMessageDefinitions_), std::move(vEnumDefinitions_))
    {
        UpdatePythonEnums();
    }

    explicit PyMessageDatabase(const MessageDatabase& message_db) : MessageDatabase(message_db) { UpdatePythonEnums(); }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdMap() const { return enums_by_id; }

  private:
    void GenerateMappings() override
    {
        MessageDatabase::GenerateMappings();
        UpdatePythonEnums();
    }

    void UpdatePythonEnums()
    {
        nb::object IntEnum = nb::module_::import_("enum").attr("IntEnum");
        enums_by_id.clear();
        for (const auto& enum_def : EnumDefinitions())
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

    std::unordered_map<std::string, nb::object> enums_by_id{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;
};

} // namespace novatel::edie