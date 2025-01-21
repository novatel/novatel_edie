#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"

namespace nb = nanobind;

namespace novatel::edie {

class PyMessageDatabase final : public MessageDatabase
{
  public:
    PyMessageDatabase();
    PyMessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_);
    explicit PyMessageDatabase(const MessageDatabase& message_db) noexcept;
    explicit PyMessageDatabase(const MessageDatabase&& message_db) noexcept;

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return enums_by_id; }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return enums_by_name; }
    std::unordered_map<std::string, nb::object> message_types{};


  private:
    void GenerateMappings() override;
    void UpdatePythonEnums();
    void UpdateMessageTypes();

    std::unordered_map<std::string, nb::object> enums_by_id{};
    std::unordered_map<std::string, nb::object> enums_by_name{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;
};

} // namespace novatel::edie
