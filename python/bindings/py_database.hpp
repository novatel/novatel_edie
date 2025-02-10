#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"

namespace nb = nanobind;

namespace novatel::edie {

struct PyMessageType
{
    nb::object python_type;
    uint32_t crc;

    PyMessageType(nb::object python_type_, uint32_t crc_) : python_type(std::move(python_type_)), crc(crc_) {}
};

class PyMessageDatabase final : public MessageDatabase
{
  public:
    PyMessageDatabase();
    PyMessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_);
    explicit PyMessageDatabase(const MessageDatabase& message_db) noexcept;
    explicit PyMessageDatabase(const MessageDatabase&& message_db) noexcept;

    [[nodiscard]] const std::unordered_map<std::string, PyMessageType*>& GetMessagesByNameDict() const { return messages_by_name; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetFieldsByNameDict() const { return fields_by_name; }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return enums_by_id; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return enums_by_name; }



  private:
    void GenerateMappings() override;
    //-----------------------------------------------------------------------
    //! \brief Creates Python Enums for each enum definition in the database.
    //!
    //! These classes are stored by ID in the enums_by_id map and by name in the enums_by_name map.
    //-----------------------------------------------------------------------
    void UpdatePythonEnums();
    //-----------------------------------------------------------------------
    //! \brief Creates Python types for each component of all message definitions in the database.
    //! 
    //! A message named "MESSAGE" will be mapped to a Python class named "MESSAGE".
    //! A field of that body named "FIELD" will be mapped to a class named "MESSAGE_FIELD_Field".
    //! A subfield of that field named "SUBFIELD" will be mapped to a class named "MESSAGE_FIELD_Field_SUBFIELD_Field".
    //! 
    //! These classes are stored by name in the messages_by_name map.
    //-----------------------------------------------------------------------
    void UpdatePythonMessageTypes();
    void AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, nb::handle type_cons, nb::handle type_tuple, nb::handle type_dict);

    std::unordered_map <std::string, PyMessageType*> messages_by_name{};
    std::unordered_map<std::string, nb::object> fields_by_name{};

    std::unordered_map<std::string, nb::object> enums_by_id{};
    std::unordered_map<std::string, nb::object> enums_by_name{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;
};

} // namespace novatel::edie
