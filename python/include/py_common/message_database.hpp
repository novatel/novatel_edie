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

class MessageDBExtrasBase
{
  public:
    virtual ~MessageDBExtrasBase() = default;
};

using MessageFamilyExtrasAllocFn = std::unique_ptr<MessageDBExtrasBase> (*)(std::shared_ptr<MessageDatabase> database);

struct MessageFamilyRegistration
{
    nb::handle messageType;
    MessageFamilyExtrasAllocFn allocateExtras;
};

std::unordered_map<std::string, MessageFamilyRegistration>& GetMessageFamilyRegistrations();
const MessageFamilyRegistration* GetMessageFamilyRegistration(const std::string& message_family);

//============================================================================
//! \class PyMessageDatabase
//! \brief A Python-facing message database that owns a MessageDatabase, the
//!        Python type caches built from it, and any registered message-family
//!        extras (e.g. OEM Encoder, RxConfigHandler).
//============================================================================
class PyMessageDatabase
{
  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;

    explicit PyMessageDatabase(MessageDatabase&& message_db = MessageDatabase());

    PyMessageDatabase(const PyMessageDatabase&) = delete;
    PyMessageDatabase& operator=(const PyMessageDatabase&) = delete;
    PyMessageDatabase(PyMessageDatabase&&) = delete;
    PyMessageDatabase& operator=(PyMessageDatabase&&) = delete;

    [[nodiscard]] const MessageDatabase::Ptr& core() const { return core_; }
    [[nodiscard]] const MessageDBExtrasBase* GetMessageFamilyExtras() const { return extras_.get(); }

    // Mutators — wrap MessageDatabase mutations so the Python caches stay in sync.
    void Merge(const Ptr& other);
    void AppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_);
    void AppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_);
    void RemoveMessage(uint32_t iMsgId_);
    void RemoveEnumeration(std::string strEnumeration_);

    // Definition lookups — forward to the underlying MessageDatabase.
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(std::string_view name) const { return core_->GetMsgDef(name); }
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(int32_t id) const { return core_->GetMsgDef(id); }
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefId(const std::string& id) const { return core_->GetEnumDefId(id); }
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefName(const std::string& name) const { return core_->GetEnumDefName(name); }
    [[nodiscard]] std::string MsgIdToMsgName(uint32_t id) const { return core_->MsgIdToMsgName(id); }

    // Python type-cache lookups.
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

    [[nodiscard]] const MessageFamilyRegistration* GetMessageFamilyRegistration() const { return message_family_registration_; }

    void bindFieldsToModule(nanobind::module_& m_, const std::vector<BaseField::ConstPtr>& fields_)
    {
        for (const auto& field : fields_)
        {
            if (field->type == FIELD_TYPE::FIELD_ARRAY)
            {
                auto* fieldArrayField = dynamic_cast<const FieldArrayField*>(field.get());
                nb::handle fieldType = field_types.at(fieldArrayField);
                m_.attr(fieldType.attr("__name__")) = fieldType;
                bindFieldsToModule(m_, fieldArrayField->fieldInfo.messageOrderedFields);
            }
        }
    }

    void addFieldAliasToModule(nanobind::module_& m_, const std::vector<BaseField::ConstPtr>& fields_, const std::string& parentAlias_)
    {
        for (const auto& field : fields_)
        {
            if (field->type == FIELD_TYPE::FIELD_ARRAY)
            {
                auto* fieldArrayField = dynamic_cast<const FieldArrayField*>(field.get());
                nb::handle fieldType = field_types[fieldArrayField];
                std::string alias = parentAlias_ + "_" + fieldArrayField->name;
                m_.attr(alias.c_str()) = fieldType;
                addFieldAliasToModule(m_, fieldArrayField->fieldInfo.messageOrderedFields, alias);
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
                bindFieldsToModule(messageMod_, message_def->fieldInfo.at(crc).messageOrderedFields);
            }
            const std::vector<BaseField::ConstPtr>& latestDef = message_def->fieldInfo.at(message_def->latestMessageCrc).messageOrderedFields;
            messageMod_.attr(message_def->name.c_str()) = message_version_defs.at(message_def->latestMessageCrc);
            addFieldAliasToModule(messageMod_, latestDef, message_def->name);
        }
        for (const auto& [enum_def, enum_type] : enum_types) { enumsMod_.attr(enum_def->name.c_str()) = enum_type; }
    }

    [[nodiscard]] Ptr clone() const;

  private:
    void Initialize();
    void ResolveBaseType();
    void allocateExtras();

    //-----------------------------------------------------------------------
    //! \brief Creates Python Enums for multiple enum definitions.
    //-----------------------------------------------------------------------
    void AppendEnumTypes(const std::vector<EnumDefinition::ConstPtr>& enum_defs);
    void RemoveEnumType(const std::string& enum_name);
    //-----------------------------------------------------------------------
    //! \brief Creates Python types for multiple message definitions and their fields.
    //-----------------------------------------------------------------------
    void AppendMessageTypes(const std::vector<MessageDefinition::ConstPtr>& message_defs);
    void RemoveMessageType(uint32_t message_id);
    void RemoveFieldTypes(const std::vector<BaseField::ConstPtr>& fieldDefs);

    void UpdatePythonEnums();
    void UpdatePythonMessageTypes();
    void AddFieldType(std::vector<BaseField::ConstPtr> fields, std::string base_name, std::string parent_message, nb::handle type_cons,
                      nb::handle type_tuple, nb::handle type_dict);

    const MessageFamilyRegistration* message_family_registration_ = nullptr;
    std::unordered_map<const MessageDefinition*, std::map<uint32_t, nb::object>> messages_types{};
    std::unordered_map<const BaseField*, nb::object> field_types{};
    std::unordered_map<const EnumDefinition*, nb::object> enum_types{};
    std::unordered_map<const BaseField*, FieldNameMap> field_name_maps_{};
    std::unordered_map<const MessageDefinition*, std::map<uint32_t, FieldNameMap>> message_field_name_maps_{};

    std::unique_ptr<MessageDBExtrasBase> extras_;
    MessageDatabase::Ptr core_;
};

} // namespace novatel::edie::py_common
