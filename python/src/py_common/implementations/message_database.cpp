#include "py_common/message_database.hpp"

#include <functional>

#include <nanobind/stl/unordered_map.h>

#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/field_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

PYCOMMON_EXPORT py_common::PyMessageDatabaseCore::PyMessageDatabaseCore()
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT py_common::PyMessageDatabaseCore::PyMessageDatabaseCore(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_,
                                                                        std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_)
    : MessageDatabase(std::move(vMessageDefinitions_), std::move(vEnumDefinitions_))
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT py_common::PyMessageDatabaseCore::PyMessageDatabaseCore(const MessageDatabase& message_db) noexcept : MessageDatabase(message_db)
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT py_common::PyMessageDatabaseCore::PyMessageDatabaseCore(const MessageDatabase&& message_db) noexcept : MessageDatabase(message_db)
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::GenerateMessageMappings()
{
    MessageDatabase::GenerateMessageMappings();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::GenerateEnumMappings()
{
    MessageDatabase::GenerateEnumMappings();
    UpdatePythonEnums();
}

void cleanString(std::string& str)
{
    // Remove special characters from the string to make it a valid python attribute name
    for (char& c : str)
    {
        if (!isalnum(c)) { c = '_'; }
    }
    if (isdigit(str[0])) { str = "_" + str; }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::UpdatePythonEnums()
{
    enums_by_id.clear();
    enums_by_name.clear();
    AppendEnumTypes(EnumDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::AppendEnumTypes(const std::vector<EnumDefinition::ConstPtr>& enum_defs)
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
        nb::object enum_type = IntEnum(enum_name, values);
        enum_type.attr("_name") = enum_name;
        enum_type.attr("_id") = enum_def->_id;
        enums_by_id[enum_def->_id.c_str()] = enum_type;
        enums_by_name[enum_name] = enum_type;
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::RemoveEnumType(const std::string& enum_name)
{
    // get the enum definition to retrieve the name
    EnumDefinition::ConstPtr enum_def = GetEnumDefName(enum_name);
    if (enum_def)
    {
        // remove from both maps
        enums_by_id.erase(enum_def->_id);
        enums_by_name.erase(enum_def->name);
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name,
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
            fields_by_name[field_name] = field_type;
            fields_by_message[parent_message].push_back(field_name);
            AddFieldType(field_array_field->fields, field_name, parent_message, type_constructor, type_tuple, type_dict);
        }
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::UpdatePythonMessageTypes()
{
    // clear existing definitions
    messages_by_name.clear();

    // add message and message body types for each message definition
    AppendMessageTypes(MessageDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::AppendMessageTypes(const std::vector<MessageDefinition::ConstPtr>& message_defs)
{
    // get type constructor
    nb::object type_constructor = nb::module_::import_("builtins").attr("type");
    // specify the python superclass for the message body types
    nb::tuple field_type_tuple = nb::make_tuple(nb::type<py_common::PyField>());
    // provide no additional attributes via `__dict__`
    nb::dict type_dict = nb::dict();

    // add message and message body types for each message definition
    for (const auto& message_def : message_defs)
    {
        // remove existing message type if it exists to ensure clean overwrite
        RemoveMessageType(message_def->logID);

        // specify the python superclass for the message type
        nb::tuple message_type_tuple = nb::make_tuple(GetBasePythonType(message_def->messageStyle));
        uint32_t crc = message_def->latestMessageCrc;
        nb::object msg_type_def = type_constructor(message_def->name, message_type_tuple, type_dict);
        messages_by_name[message_def->name] = PyMessageType(msg_type_def, crc);
        // add additional MessageBody types for each field array element within the message definition
        AddFieldType(message_def->fields.at(crc), message_def->name, message_def->name, type_constructor, field_type_tuple, type_dict);
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabaseCore::RemoveMessageType(uint32_t message_id)
{
    // get the message definition to retrieve the name
    MessageDefinition::ConstPtr message_def = GetMsgDef(message_id);
    if (!message_def) { return; }

    const std::string& message_name = message_def->name;

    // remove the message type
    messages_by_name.erase(message_name);

    // remove all field types associated with this message from fields_by_name
    auto fields_it = fields_by_message.find(message_name);
    if (fields_it != fields_by_message.end())
    {
        for (const std::string& field_name : fields_it->second)
        {
            // remove from fields_by_name
            fields_by_name.erase(field_name);
        }
        // remove from fields_by_message
        fields_by_message.erase(fields_it);
    }
}

PYCOMMON_EXPORT std::unordered_map<std::string, std::function<nb::handle()>>& py_common::GetBasePythonTypes()
{
    static std::unordered_map<std::string, std::function<nb::handle()>> nameToTypeGetter;
    return nameToTypeGetter;
}

PYCOMMON_EXPORT nb::handle py_common::GetBasePythonType(const std::string& message_style)
{
    std::unordered_map<std::string, std::function<nb::handle()>>& typeGetters = GetBasePythonTypes();
    std::function<nb::handle()> typeGetter = typeGetters.at(message_style);
    return typeGetter();
}
