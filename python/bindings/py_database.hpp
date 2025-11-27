#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"
#include "exceptions.hpp"


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

  private:
    void GenerateMessageMappings() override;
    void GenerateEnumMappings() override;

    void AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, nb::handle type_cons, nb::handle type_tuple,
                      nb::handle type_dict);

    std::unordered_map<std::string, PyMessageType*> messages_by_name{};
    std::unordered_map<std::string, nb::object> fields_by_name{};

    std::unordered_map<std::string, nb::object> enums_by_id{};
    std::unordered_map<std::string, nb::object> enums_by_name{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabaseCore>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabaseCore>;
};

class PyMessageDatabase
{
  public:
    PyMessageDatabase();
    PyMessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_);
    explicit PyMessageDatabase(const MessageDatabase& message_db);
    explicit PyMessageDatabase(const MessageDatabase&& message_db);

    [[nodiscard]] std::string MsgIdToMsgName(uint32_t uiMessageId_) const { return pclMessageDb->MsgIdToMsgName(uiMessageId_); };

    void PyAppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_) { pclMessageDb->AppendMessages(vMessageDefinitions_); pclMessageDb->UpdatePythonMessageTypes(); }

    void PyAppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_) { pclMessageDb->AppendEnumerations(vEnumDefinitions_); pclMessageDb->UpdatePythonEnums(); }

    void PyRemoveMessage(const uint32_t iMsgId_) { pclMessageDb->RemoveMessage(iMsgId_); pclMessageDb->UpdatePythonMessageTypes(); }

    void PyRemoveEnumeration(std::string_view strEnumeration_) { pclMessageDb->RemoveEnumeration(strEnumeration_); pclMessageDb->UpdatePythonEnums(); }

    // MessageDatabase wrappers
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(std::string_view strMsgName_) const { return pclMessageDb->GetMsgDef(strMsgName_); }
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(int32_t iMsgId_) const { return pclMessageDb->GetMsgDef(iMsgId_); }

    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefId(std::string& strEnumId_) const { return pclMessageDb->GetEnumDefId(strEnumId_); }
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefName(std::string& strEnumName_) const { return pclMessageDb->GetEnumDefName(strEnumName_); }

    void Merge(const MessageDatabase& other_) { pclMessageDb->Merge(other_); }

    // PyMessageDatabaseCore wrappers
    [[nodiscard]] const std::unordered_map<std::string, PyMessageType*>& GetMessagesByNameDict() const { return pclMessageDb->GetMessagesByNameDict(); }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetFieldsByNameDict() const { return pclMessageDb->GetFieldsByNameDict(); }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return pclMessageDb->GetEnumsByIdDict(); }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return pclMessageDb->GetEnumsByNameDict(); }

  private:
    PyMessageDatabaseCore::Ptr pclMessageDb; // This is the MessageDatabase that this class wraps
    std::unique_ptr<oem::Encoder> pclEncoder;
    std::unique_ptr<oem::RxConfigHandler> pclRxConfigHandler;

    void ThrowRXConfigConstructionError() const
    {
        throw FailureException("RXConfig definition in database cannot be handled.");
    }

  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;

    PyMessageDatabaseCore::Ptr GetCoreDatabase() const { return pclMessageDb; }
    const oem::Encoder* GetEncoder() const { return pclEncoder.get(); }
    oem::RxConfigHandler* GetRxConfigHandler() const { return pclRxConfigHandler.get(); }
};

} // namespace novatel::edie
