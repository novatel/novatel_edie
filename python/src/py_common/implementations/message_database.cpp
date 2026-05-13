#include "py_common/message_database.hpp"

#include <functional>
#include <iomanip>
#include <sstream>

#include <nanobind/stl/unordered_map.h>

#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/field_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

PYCOMMON_EXPORT std::unordered_map<std::string, py_common::MessageFamilyRegistration>& py_common::GetMessageFamilyRegistrations()
{
    static std::unordered_map<std::string, py_common::MessageFamilyRegistration> registrations;
    return registrations;
}

PYCOMMON_EXPORT const py_common::MessageFamilyRegistration* py_common::GetMessageFamilyRegistration(const std::string& message_family)
{
    auto& registrations = GetMessageFamilyRegistrations();
    auto registrationIt = registrations.find(message_family);
    if (registrationIt == registrations.end()) { return nullptr; }
    return &registrationIt->second;
}

PYCOMMON_EXPORT py_common::PyMessageDatabase::PyMessageDatabase(MessageDatabase&& message_db)
    : core_(std::make_shared<MessageDatabase>(std::move(message_db)))
{
    Initialize();
    allocateExtras();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::Initialize()
{
    ResolveBaseType();
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::ResolveBaseType()
{
    if (!core_->GetDbMetadata()) { core_->SetMessageFamily(""); }

    const std::string family = GetMessageFamily();
    message_family_registration_ = py_common::GetMessageFamilyRegistration(family);
    if (!message_family_registration_ || !message_family_registration_->messageType.is_valid())
    {
        throw FailureException(family + " is not a recognized message type.");
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::allocateExtras()
{
    if (message_family_registration_ && message_family_registration_->allocateExtras)
    {
        extras_ = message_family_registration_->allocateExtras(core_);
    }
    else { extras_.reset(); }
}

PYCOMMON_EXPORT std::string py_common::PyMessageDatabase::GetMessageFamily() const
{
    auto metadata = core_->GetDbMetadata();
    return metadata ? metadata->messageFamily : "";
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::SetMessageFamily(const std::string& messageFamily)
{
    core_->SetMessageFamily(messageFamily);
    ResolveBaseType();
    UpdatePythonMessageTypes();
    allocateExtras();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::Merge(const Ptr& other_)
{
    core_->AppendEnumerations(other_->core_->EnumDefinitions());
    AppendEnumTypes(other_->core_->EnumDefinitions());
    core_->AppendMessages(other_->core_->MessageDefinitions());
    AppendMessageTypes(other_->core_->MessageDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_)
{
    core_->AppendMessages(vMessageDefinitions_);
    AppendMessageTypes(vMessageDefinitions_);
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_)
{
    core_->AppendEnumerations(vEnumDefinitions_);
    AppendEnumTypes(vEnumDefinitions_);
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveMessage(uint32_t iMsgId_)
{
    RemoveMessageType(iMsgId_);
    core_->RemoveMessage(iMsgId_);
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveEnumeration(std::string strEnumeration_)
{
    RemoveEnumType(strEnumeration_);
    core_->RemoveEnumeration(strEnumeration_);
}

PYCOMMON_EXPORT py_common::PyMessageDatabase::Ptr py_common::PyMessageDatabase::clone() const
{
    return std::make_shared<PyMessageDatabase>(MessageDatabase(*core_));
}

static void cleanString(std::string& str)
{
    // Remove special characters from the string to make it a valid python attribute name
    for (char& c : str)
    {
        if (!isalnum(c)) { c = '_'; }
    }
    if (isdigit(str[0])) { str = "_" + str; }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::UpdatePythonEnums()
{
    enum_types.clear();
    AppendEnumTypes(core_->EnumDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendEnumTypes(const std::vector<EnumDefinition::ConstPtr>& enum_defs)
{
    nb::object IntEnum = nb::module_::import_("enum").attr("IntEnum");
    for (const auto& enum_def : enum_defs)
    {
        // remove existing enum type if it exists to ensure clean overwrite
        RemoveEnumType(enum_def->name);

        nb::dict values;
        const char* enum_name = enum_def->name.c_str();
        for (const auto& enumerator : enum_def->enumerators)
        {
            std::string enumerator_name = enumerator.name;
            cleanString(enumerator_name);
            values[enumerator_name.c_str()] = enumerator.value;
        }
        values["EDIE_UNKNOWN"] = enum_def->unknownValue;
        nb::object enum_type = IntEnum(enum_name, values);
        enum_type.attr("_name") = enum_name;
        enum_type.attr("_id") = enum_def->_id;
        enum_types[enum_def.get()] = enum_type;
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveEnumType(const std::string& enum_name)
{
    // get the enum definition to retrieve the pointer
    EnumDefinition::ConstPtr enum_def = core_->GetEnumDefName(enum_name);
    if (enum_def) { enum_types.erase(enum_def.get()); }
}

static py_common::FieldNameMap BuildFieldNameMapFromDefs(const std::vector<BaseField::Ptr>& fields)
{
    py_common::FieldNameMap map;
    for (size_t i = 0; i < fields.size(); i++)
    {
        const auto& def = fields[i];
        map[def->name] = {i, false};
        if (def->type == FIELD_TYPE::FIELD_ARRAY || def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY) { map[def->name + "_length"] = {i, true}; }
    }
    return map;
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::UpdatePythonMessageTypes()
{
    // clear existing definitions
    messages_types.clear();
    field_name_maps_.clear();
    message_field_name_maps_.clear();

    // add message and message body types for each message definition
    AppendMessageTypes(core_->MessageDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name,
                                                                std::string parent_message, nb::handle type_constructor, nb::handle type_tuple,
                                                                nb::handle type_dict)
{
    // rescursively add field types for each field array element within the provided vector
    for (const auto& field : fields)
    {
        if (field->type == FIELD_TYPE::FIELD_ARRAY)
        {
            auto* field_array_field = dynamic_cast<FieldArrayField*>(field.get());
            std::string field_name = base_name + "_" + field_array_field->name + "_Field";
            nb::object field_type = type_constructor(field_name, type_tuple, type_dict);
            field_types[field.get()] = field_type;
            field_name_maps_[field.get()] = BuildFieldNameMapFromDefs(field_array_field->fields);
            AddFieldType(field_array_field->fields, field_name, parent_message, type_constructor, type_tuple, type_dict);
        }
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendMessageTypes(const std::vector<MessageDefinition::ConstPtr>& message_defs)
{
    if (!message_family_registration_ || !message_family_registration_->messageType.is_valid())
    {
        throw FailureException("Message family type is not initialized.");
    }

    // get type constructor
    nb::object type_constructor = nb::module_::import_("builtins").attr("type");
    // specify the python superclass for the message body types
    nb::tuple field_type_tuple = nb::make_tuple(nb::type<py_common::PyField>());
    // provide no additional attributes via `__dict__`
    nb::dict type_dict = nb::dict();
    nb::tuple message_type_tuple = nb::make_tuple(message_family_registration_->messageType);
    type_dict[nb::str("__slots__")] = nb::make_tuple();

    // add message and message body types for each message definition
    for (const auto& message_def : message_defs)
    {
        // Initialize message mapping
        // remove existing message type if it exists to ensure clean overwrite
        RemoveMessageType(message_def->logID);
        messages_types[message_def.get()] = std::map<uint32_t, nb::object>{};

        for (const auto& [crc, message_fields] : message_def->fields)
        {
            std::ostringstream ss;
            ss << message_def->name << "_" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << crc;
            std::string message_name = ss.str();

            // specify the python superclass for the message type
            nb::object msg_py_type = type_constructor(message_name, message_type_tuple, type_dict);
            messages_types[message_def.get()][crc] = msg_py_type;
            message_field_name_maps_[message_def.get()][crc] = BuildFieldNameMapFromDefs(message_fields);

            AddFieldType(message_fields, message_name, message_name, type_constructor, field_type_tuple, type_dict);
        }
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveMessageType(uint32_t message_id)
{
    // get the message definition to retrieve the name
    MessageDefinition::ConstPtr message_def = core_->GetMsgDef(message_id);
    if (!message_def) { return; }

    // remove the message type
    messages_types.erase(message_def.get());
    message_field_name_maps_.erase(message_def.get());

    // remove all field types associated with this message from field_types
    for (const auto& [crc, fields] : message_def->fields) { RemoveFieldTypes(fields); }
}

void py_common::PyMessageDatabase::RemoveFieldTypes(const std::vector<BaseField::Ptr>& fieldDefs)
{
    for (const auto& fieldDef : fieldDefs)
    {
        field_types.erase(fieldDef.get());
        field_name_maps_.erase(fieldDef.get());
        if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
        {
            auto* fieldArrayFieldDef = dynamic_cast<FieldArrayField*>(fieldDef.get());
            RemoveFieldTypes(fieldArrayFieldDef->fields);
        }
    }
}
