#pragma once

#include <functional>

#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_common {

std::unordered_map<std::string, nb::handle>& GetMessageFamilyTypes();
nb::handle GetMessageFamilyType(const std::string& message_family);

struct PyMessageType
{
    nb::object python_type;
    uint32_t crc;

    PyMessageType() {}
    PyMessageType(nb::object python_type_, uint32_t crc_) : python_type(std::move(python_type_)), crc(crc_) {}
};

class PyMessageDatabaseCore : public MessageDatabase
{
  public:
    PyMessageDatabaseCore();
    explicit PyMessageDatabaseCore(const MessageDatabase& message_db) noexcept;
    explicit PyMessageDatabaseCore(const MessageDatabase&& message_db) noexcept;

    [[nodiscard]] nb::object GetFieldType(const BaseField* field) const
    {
        auto it = fields_by_def.find(field);
        if (it == fields_by_def.end()) { throw std::out_of_range("Field type not found"); }
        return it->second;
    }

    [[nodiscard]] nb::object GetMessageType(const MessageDefinition* message)
    {
        if (message == nullptr) { return nb::none(); }
        return GetMessageType(message, message->latestMessageCrc);
    }

    template <bool crcFallback = false> [[nodiscard]] nb::object GetMessageType(const MessageDefinition* message, uint32_t crc)
    {
        auto it = messages_by_name.find(message);
        if (it == messages_by_name.end()) { return nb::none(); }
        auto nestedIt = it->second.find(crc);
        if (nestedIt == it->second.end())
        {
            if constexpr (crcFallback) { return it->second.at(message->latestMessageCrc); }
            else { return nb::none(); }
        }
        return nestedIt->second;
    }

    template <bool crcFallback = false> [[nodiscard]] nb::object GetMessageType(std::string_view messageName, uint32_t crc)
    {
        return GetMessageType<crcFallback>(GetMsgDef(messageName).get(), crc);
    }
    [[nodiscard]] nb::object GetMessageType(std::string_view messageName) { return GetMessageType(GetMsgDef(messageName).get()); }

    [[nodiscard]] const std::unordered_map<const BaseField*, nb::object> GetFieldsByDefDict() const { return fields_by_def; }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return enums_by_id; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return enums_by_name; }

    [[nodiscard]] std::string GetMessageFamily() const;
    void SetMessageFamily(const std::string& messageFamily);

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
                nb::handle fieldType = fields_by_def[fieldArrayField];
                fieldType.attr("__module__") = m_.attr("__name__");
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
                nb::handle fieldType = fields_by_def[fieldArrayField];
                std::string alias = parentAlias_ + "_" + fieldArrayField->name;
                m_.attr(alias.c_str()) = fieldType;
                addFieldAliasToModule(m_, fieldArrayField->fields, alias);
            }
        }
    }

    void bindToModule(nanobind::module_& messageMod_, nanobind::module_& enumsMod_)
    {
        for (const auto& [message_def, message_version_defs] : messages_by_name)
        {
            for (const auto& [crc, message_type] : message_version_defs)
            {
                message_type.attr("__module__") = messageMod_.attr("__name__");
                std::string name = nb::cast<nb::str>(message_type.attr("__name__")).c_str();
                messageMod_.attr(message_type.attr("__name__")) = message_type;
                bindFieldsToModule(messageMod_, message_def->fields.at(crc));
            }
            const std::vector<BaseField::Ptr>& latestDef = message_def->fields.at(message_def->latestMessageCrc);
            messageMod_.attr(message_def->name.c_str()) = message_version_defs.at(message_def->latestMessageCrc);
            addFieldAliasToModule(messageMod_, latestDef, message_def->name);
        }
        for (const auto& [name, enum_type] : enums_by_name) { enumsMod_.attr(name.c_str()) = enum_type; }
    }

  private:
    //-----------------------------------------------------------------------
    //! \brief Creates Python Enums for multiple enum definitions.
    //!
    //! These classes are stored by ID in the enums_by_id map and by name in the enums_by_name map.
    //-----------------------------------------------------------------------
    void AppendEnumTypes(const std::vector<EnumDefinition::ConstPtr>& enum_defs);
    //-----------------------------------------------------------------------
    //! \brief Removes an enum type from the Python type maps.
    //!
    //! Removes the enum from both enums_by_id and enums_by_name.
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
    void AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, std::string parent_message, nb::handle type_cons,
                      nb::handle type_tuple, nb::handle type_dict);

    void ResolveBaseType();

    nb::handle base_message_type;
    std::unordered_map<const MessageDefinition*, std::map<uint32_t, nb::object>> messages_by_name{};
    std::unordered_map<const BaseField*, nb::object> fields_by_def{};

    std::unordered_map<std::string, nb::object> enums_by_id{};
    std::unordered_map<std::string, nb::object> enums_by_name{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabaseCore>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabaseCore>;
};

} // namespace novatel::edie::py_common
