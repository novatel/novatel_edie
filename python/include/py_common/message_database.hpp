#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_common {

struct FieldLookupEntry
{
    size_t index;
    bool is_length;
};

using FieldNameMap = std::unordered_map<std::string, FieldLookupEntry>;

class PyMessageDatabaseCore;

using MessageFamilyExtrasAllocFn = void* (*)(PyMessageDatabaseCore* database);
using MessageFamilyExtrasFreeFn = void (*)(void* extras);

struct MessageFamilyRegistration
{
    nb::handle messageType;
    MessageFamilyExtrasAllocFn allocateExtras;
    MessageFamilyExtrasFreeFn freeExtras;
};

std::unordered_map<std::string, MessageFamilyRegistration>& GetMessageFamilyRegistrations();
const MessageFamilyRegistration* GetMessageFamilyRegistration(const std::string& message_family);

class PyMessageDatabaseCore : public MessageDatabase
{
  public:
    // All python message databases are to be managed via a shared pointer
    static std::shared_ptr<PyMessageDatabaseCore> Create(MessageDatabase message_db = MessageDatabase());
    ~PyMessageDatabaseCore();

    // Only allow construction via static Create method
    PyMessageDatabaseCore(const PyMessageDatabaseCore&) = delete;
    PyMessageDatabaseCore& operator=(const PyMessageDatabaseCore&) = delete;
    PyMessageDatabaseCore(PyMessageDatabaseCore&&) = delete;
    PyMessageDatabaseCore& operator=(PyMessageDatabaseCore&&) = delete;

    [[nodiscard]] nb::object GetFieldType(const BaseField* field) const
    {
        auto it = field_types.find(field);
        if (it == field_types.end()) { return nb::none(); }
        return it->second;
    }

    [[nodiscard]] nb::object GetMessageType(const MessageDefinition* message, uint32_t crc) const
    {
        auto it = messages_types.find(message);
        if (it == messages_types.end()) { return nb::none(); }
        auto nestedIt = it->second.find(crc);
        if (nestedIt == it->second.end()) { return nb::none(); }
        return nestedIt->second;
    }
    [[nodiscard]] nb::object GetMessageType(const MessageDefinition* message) const
    {
        if (message == nullptr) { return nb::none(); }
        return GetMessageType(message, message->latestMessageCrc);
    }

    [[nodiscard]] nb::object GetMessageType(std::string_view messageName, uint32_t crc) const
    {
        return GetMessageType(GetMsgDef(messageName).get(), crc);
    }
    [[nodiscard]] nb::object GetMessageType(std::string_view messageName) const { return GetMessageType(GetMsgDef(messageName).get()); }

    [[nodiscard]] nb::object GetMessageType(int32_t id, uint32_t crc) const { return GetMessageType(GetMsgDef(id).get(), crc); }
    [[nodiscard]] nb::object GetMessageType(int32_t id) const { return GetMessageType(GetMsgDef(id).get()); }

    [[nodiscard]] const std::unordered_map<const BaseField*, nb::object> GetFieldsByDefDict() const { return field_types; }

    [[nodiscard]] const FieldNameMap* GetFieldNameMap(const BaseField* field) const
    {
        auto it = field_name_maps_.find(field);
        return it != field_name_maps_.end() ? &it->second : nullptr;
    }

    [[nodiscard]] const FieldNameMap* GetMessageFieldNameMap(const MessageDefinition* msgDef, uint32_t crc) const
    {
        auto it = message_field_name_maps_.find(msgDef);
        if (it == message_field_name_maps_.end()) { return nullptr; }
        auto nestedIt = it->second.find(crc);
        if (nestedIt == it->second.end()) { return nullptr; }
        return &nestedIt->second;
    }

    [[nodiscard]] nb::object GetEnumType(const EnumDefinition* enum_def) const
    {
        if (enum_def == nullptr) { return nb::none(); }
        auto it = enum_types.find(enum_def);
        return it == enum_types.end() ? nb::none() : it->second;
    }
    [[nodiscard]] nb::object GetEnumTypeByName(const std::string& name) const { return GetEnumType(GetEnumDefName(name).get()); }
    [[nodiscard]] nb::object GetEnumTypeById(const std::string& id) const { return GetEnumType(GetEnumDefId(id).get()); }

    [[nodiscard]] std::string GetMessageFamily() const;
    void SetMessageFamily(const std::string& messageFamily);

    [[nodiscard]] void* GetMessageFamilyExtras() const { return message_family_extras_; }
    [[nodiscard]] std::shared_ptr<PyMessageDatabaseCore> GetSharedPtr() const { return self_weak_.lock(); }

    // MessageDatabase overloads
    void Merge(const std::shared_ptr<PyMessageDatabaseCore> other_);
    void AppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_);
    void AppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_);
    void RemoveMessage(uint32_t iMsgId_);
    void RemoveFieldTypes(const std::vector<BaseField::Ptr>& fieldDefs);
    void RemoveEnumeration(std::string strEnumeration_);

    void bindFieldsToModule(nanobind::module_& m_, const std::vector<BaseField::Ptr>& fields_)
    {
        for (const auto& field : fields_)
        {
            if (field->type == FIELD_TYPE::FIELD_ARRAY)
            {
                auto* fieldArrayField = dynamic_cast<FieldArrayField*>(field.get());
                nb::handle fieldType = field_types.at(fieldArrayField);
                m_.attr(fieldType.attr("__name__")) = fieldType;
                bindFieldsToModule(m_, fieldArrayField->fields);
            }
        }
    }

    void addFieldAliasToModule(nanobind::module_& m_, const std::vector<BaseField::Ptr>& fields_, const std::string& parentAlias_)
    {
        for (const auto& field : fields_)
        {
            if (field->type == FIELD_TYPE::FIELD_ARRAY)
            {
                auto* fieldArrayField = dynamic_cast<FieldArrayField*>(field.get());
                nb::handle fieldType = field_types[fieldArrayField];
                std::string alias = parentAlias_ + "_" + fieldArrayField->name;
                m_.attr(alias.c_str()) = fieldType;
                addFieldAliasToModule(m_, fieldArrayField->fields, alias);
            }
        }
    }

    void bindToModule(nanobind::module_& messageMod_, nanobind::module_& enumsMod_)
    {
        for (const auto& [message_def, message_version_defs] : messages_types)
        {
            for (const auto& [crc, message_type] : message_version_defs)
            {
                std::string name = nb::cast<nb::str>(message_type.attr("__name__")).c_str();
                messageMod_.attr(message_type.attr("__name__")) = message_type;
                bindFieldsToModule(messageMod_, message_def->fields.at(crc));
            }
            const std::vector<BaseField::Ptr>& latestDef = message_def->fields.at(message_def->latestMessageCrc);
            messageMod_.attr(message_def->name.c_str()) = message_version_defs.at(message_def->latestMessageCrc);
            addFieldAliasToModule(messageMod_, latestDef, message_def->name);
        }
        for (const auto& [enum_def, enum_type] : enum_types) { enumsMod_.attr(enum_def->name.c_str()) = enum_type; }
    }

  private:
    explicit PyMessageDatabaseCore(MessageDatabase&& message_db);

    void Initialize();
    void ResetMessageFamilyExtras();

    //-----------------------------------------------------------------------
    //! \brief Creates Python Enums for multiple enum definitions.
    //!
    //! These classes are stored by EnumDefinition pointer in the enum_types map.
    //-----------------------------------------------------------------------
    void AppendEnumTypes(const std::vector<EnumDefinition::ConstPtr>& enum_defs);
    //-----------------------------------------------------------------------
    //! \brief Removes an enum type from the Python type map.
    //-----------------------------------------------------------------------
    void RemoveEnumType(const std::string& enum_name);
    //-----------------------------------------------------------------------
    //! \brief Creates Python types for multiple message definitions and their fields.
    //!
    //! A message named "MESSAGE" will be mapped to a Python class named "MESSAGE".
    //! A field of that payload named "FIELD" will be mapped to a class named "MESSAGE_FIELD_Field".
    //! A subfield of that field named "SUBFIELD" will be mapped to a class named "MESSAGE_FIELD_Field_SUBFIELD_Field".
    //!
    //! These classes are stored by name in the messages_by_name and fields_by_message maps.
    //-----------------------------------------------------------------------
    void AppendMessageTypes(const std::vector<MessageDefinition::ConstPtr>& message_defs);
    //-----------------------------------------------------------------------
    //! \brief Removes a message type and its associated field types from the Python type maps.
    //!
    //! Removes the message from messages_by_name and all its associated fields from
    //! fields_by_name and fields_by_message.
    //-----------------------------------------------------------------------
    void RemoveMessageType(uint32_t message_id);

    void GenerateMessageMappings() override;
    void GenerateEnumMappings() override;
    //-----------------------------------------------------------------------
    //! \brief Creates Python Enums for each enum definition in the database.
    //!
    //! These classes are stored by EnumDefinition pointer in the enum_types map.
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
    void AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, std::string parent_message, nb::handle type_cons,
                      nb::handle type_tuple, nb::handle type_dict);

    void ResolveBaseType();

    const MessageFamilyRegistration* message_family_registration_ = nullptr;
    void* message_family_extras_ = nullptr;
    std::weak_ptr<PyMessageDatabaseCore> self_weak_;
    std::unordered_map<const MessageDefinition*, std::map<uint32_t, nb::object>> messages_types{};
    std::unordered_map<const BaseField*, nb::object> field_types{};
    std::unordered_map<const EnumDefinition*, nb::object> enum_types{};
    std::unordered_map<const BaseField*, FieldNameMap> field_name_maps_{};
    std::unordered_map<const MessageDefinition*, std::map<uint32_t, FieldNameMap>> message_field_name_maps_{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabaseCore>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabaseCore>;
};

} // namespace novatel::edie::py_common
