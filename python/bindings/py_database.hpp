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

class PyMessageDatabaseCore : public MessageDatabase
{
  public:
    PyMessageDatabaseCore();
    PyMessageDatabaseCore(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_);
    explicit PyMessageDatabaseCore(const MessageDatabase& message_db) noexcept;
    explicit PyMessageDatabaseCore(const MessageDatabase&& message_db) noexcept;

    [[nodiscard]] const std::unordered_map<std::string, PyMessageType*>& GetMessagesByNameDict() const { return messages_by_name; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetFieldsByNameDict() const { return fields_by_name; }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return enums_by_id; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return enums_by_name; }

  private:
    void GenerateMessageMappings() override;
    void GenerateEnumMappings() override;
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
    //! A field of that payload named "FIELD" will be mapped to a class named "MESSAGE_FIELD_Field".
    //! A subfield of that field named "SUBFIELD" will be mapped to a class named "MESSAGE_FIELD_Field_SUBFIELD_Field".
    //!
    //! These classes are stored by name in the messages_by_name map.
    //-----------------------------------------------------------------------
    void UpdatePythonMessageTypes();
    void AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, nb::handle type_cons, nb::handle type_tuple,
                      nb::handle type_dict);

    std::unordered_map<std::string, PyMessageType*> messages_by_name{};
    std::unordered_map<std::string, nb::object> fields_by_name{};

    std::unordered_map<std::string, nb::object> enums_by_id{};
    std::unordered_map<std::string, nb::object> enums_by_name{};

    std::shared_ptr<oem::Encoder> encoder;

  public:
    std::shared_ptr<const oem::Encoder> get_encoder() const { return encoder; }

    using Ptr = std::shared_ptr<PyMessageDatabaseCore>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabaseCore>;
};

class PyMessageDatabase
{
  public:
    PyMessageDatabase();
    PyMessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_);
    explicit PyMessageDatabase(const MessageDatabase& message_db) noexcept;
    explicit PyMessageDatabase(const MessageDatabase&& message_db) noexcept;

    std::shared_ptr<const oem::Encoder> get_encoder() const { return message_db->get_encoder(); }

    [[nodiscard]] std::string MsgIdToMsgName(uint32_t uiMessageId_) const { message_db->MsgIdToMsgName(uiMessageId_); };

    void PyAppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_) { message_db->AppendMessages(vMessageDefinitions_); }

    void PyAppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_) { message_db->AppendEnumerations(vEnumDefinitions_); }

    void PyRemoveMessage(const uint32_t iMsgId_) { message_db->RemoveMessage(iMsgId_); }

    void PyRemoveEnumeration(std::string_view strEnumeration_) { message_db->RemoveEnumeration(strEnumeration_); }

    // MessageDatabase wrappers
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(std::string_view strMsgName_) const { return message_db->GetMsgDef(strMsgName_); }
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(int32_t iMsgId_) const { return message_db->GetMsgDef(iMsgId_); }

    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefId(std::string& strEnumId_) const { return message_db->GetEnumDefId(strEnumId_); }
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefName(std::string& strEnumName_) const { return message_db->GetEnumDefName(strEnumName_); }

    void Merge(const MessageDatabase& other_) { message_db->Merge(other_); }

    // PyMessageDatabaseCore wrappers
    [[nodiscard]] const std::unordered_map<std::string, PyMessageType*>& GetMessagesByNameDict() const { return message_db->GetMessagesByNameDict(); }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetFieldsByNameDict() const { return message_db->GetFieldsByNameDict(); }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return message_db->GetEnumsByIdDict(); }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return message_db->GetEnumsByNameDict(); }

  private:
    PyMessageDatabaseCore::Ptr message_db; // This is the base MessageDatabase that this class wraps

  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;

    PyMessageDatabaseCore::Ptr GetCoreDatabase() const { return message_db; }
};

} // namespace novatel::edie
