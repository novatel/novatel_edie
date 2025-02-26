#include "novatel_edie/decoders/common/message_database.hpp"

#include <nanobind/stl/unordered_map.h>

#include "bindings_core.hpp"
#include "py_database.hpp"
#include "py_decoded_message.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_common_message_database(nb::module_& m)
{
    nb::enum_<DATA_TYPE>(m, "DATA_TYPE", "Data type name string represented as an enum")
        .value("BOOL", DATA_TYPE::BOOL)
        .value("CHAR", DATA_TYPE::CHAR)
        .value("UCHAR", DATA_TYPE::UCHAR)
        .value("SHORT", DATA_TYPE::SHORT)
        .value("USHORT", DATA_TYPE::USHORT)
        .value("INT", DATA_TYPE::INT)
        .value("UINT", DATA_TYPE::UINT)
        .value("LONG", DATA_TYPE::LONG)
        .value("ULONG", DATA_TYPE::ULONG)
        .value("LONGLONG", DATA_TYPE::LONGLONG)
        .value("ULONGLONG", DATA_TYPE::ULONGLONG)
        .value("FLOAT", DATA_TYPE::FLOAT)
        .value("DOUBLE", DATA_TYPE::DOUBLE)
        .value("HEXBYTE", DATA_TYPE::HEXBYTE)
        .value("SATELLITEID", DATA_TYPE::SATELLITEID)
        .value("UNKNOWN", DATA_TYPE::UNKNOWN)
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    m.attr("str_to_DATA_TYPE") = DataTypeEnumLookup;

    nb::enum_<FIELD_TYPE>(m, "FIELD_TYPE", "Field type string represented as an enum.")
        .value("SIMPLE", FIELD_TYPE::SIMPLE)
        .value("ENUM", FIELD_TYPE::ENUM)
        .value("BITFIELD", FIELD_TYPE::BITFIELD)
        .value("FIXED_LENGTH_ARRAY", FIELD_TYPE::FIXED_LENGTH_ARRAY)
        .value("VARIABLE_LENGTH_ARRAY", FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
        .value("STRING", FIELD_TYPE::STRING)
        .value("FIELD_ARRAY", FIELD_TYPE::FIELD_ARRAY)
        .value("RESPONSE_ID", FIELD_TYPE::RESPONSE_ID)
        .value("RESPONSE_STR", FIELD_TYPE::RESPONSE_STR)
        .value("RXCONFIG_HEADER", FIELD_TYPE::RXCONFIG_HEADER)
        .value("RXCONFIG_BODY", FIELD_TYPE::RXCONFIG_BODY)
        .value("UNKNOWN", FIELD_TYPE::UNKNOWN)
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    m.attr("str_to_FIELD_TYPE") = FieldTypeEnumLookup;

    nb::class_<EnumDataType>(m, "EnumDataType", "Enum Data Type representing contents of UI DB")
        .def(nb::init())
        .def(nb::init<std::string, uint32_t, std::string>(), "name"_a, "value"_a, "description"_a = "")
        .def_rw("value", &EnumDataType::value)
        .def_rw("name", &EnumDataType::name)
        .def_rw("description", &EnumDataType::description)
        .def("__repr__", [](const EnumDataType& enum_data_type) {
            if (enum_data_type.description.empty())
                return nb::str("EnumDataType(name={!r}, value={!r})").format(enum_data_type.name, enum_data_type.value);
            return nb::str("EnumDataType(name={!r}, value={!r}, description={!r})")
                .format(enum_data_type.name, enum_data_type.value, enum_data_type.description);
        });

    nb::class_<EnumDefinition>(m, "EnumDefinition", "Enum Definition representing contents of UI DB")
        .def(nb::init())
        .def_rw("id", &EnumDefinition::_id)
        .def_rw("name", &EnumDefinition::name)
        .def_rw("enumerators", &EnumDefinition::enumerators)
        .def("__repr__", [](const EnumDefinition& enum_def) {
            return nb::str("EnumDefinition(id={!r}, name={!r}, enumerators={!r})").format(enum_def._id, enum_def.name, enum_def.enumerators);
        });

    nb::class_<BaseDataType>(m, "BaseDataType", "Struct containing basic elements of data type fields in the UI DB")
        .def(nb::init())
        .def_rw("name", &BaseDataType::name)
        .def_rw("length", &BaseDataType::length)
        .def_rw("description", &BaseDataType::description);

    nb::class_<SimpleDataType, BaseDataType>(m, "SimpleDataType", "Struct containing elements of simple data type fields in the UI DB")
        .def(nb::init())
        .def_rw("enums", &SimpleDataType::enums);

    nb::class_<BaseField>(m, "BaseField", "Struct containing elements of basic fields in the UI DB")
        .def(nb::init())
        .def(nb::init<std::string, FIELD_TYPE, std::string, size_t, DATA_TYPE>(), "name"_a, "type"_a, "conversion"_a, "length"_a, "data_type"_a)
        .def_rw("name", &BaseField::name)
        .def_rw("type", &BaseField::type)
        .def_rw("description", &BaseField::description)
        .def_rw("conversion", &BaseField::conversion)
        .def_rw("conversion_before_point", &BaseField::conversionBeforePoint)
        .def_rw("conversion_after_point", &BaseField::conversionAfterPoint)
        .def_rw("data_type", &BaseField::dataType)
        .def("set_conversion", &BaseField::SetConversion, "conversion"_a)
        .def("__repr__", [](const BaseField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            if (desc.empty() && field.conversion.empty())
            {
                return nb::str("BaseField(name={!r}, type={}, data_type={})").format(field.name, nb::cast(field.type), field.dataType.name);
            }
            return nb::str("BaseField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion);
        });

    nb::class_<EnumField, BaseField>(m, "EnumField", "Struct containing elements of enum fields in the UI DB")
        .def(nb::init())
        .def(
            "__init__",
            [](EnumField* t, std::string name, std::vector<EnumDataType> enumerators) {
                auto enum_def = std::make_shared<EnumDefinition>();
                enum_def->name = name;
                enum_def->enumerators = std::move(enumerators);
                auto* field = new (t) EnumField; // NOLINT(*.NewDeleteLeaks)
                field->name = name;
                field->enumDef = std::move(enum_def);
                field->type = FIELD_TYPE::ENUM;
            },
            "name"_a, "enumerators"_a)
        .def_rw("enum_id", &EnumField::enumId)
        .def_rw("enum_def", &EnumField::enumDef)
        .def_rw("length", &EnumField::length)
        .def("__repr__", [](const EnumField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("EnumField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, enum_id={!r}, length={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.enumId, field.length);
        });

    nb::class_<ArrayField, BaseField>(m, "ArrayField", "Struct containing elements of array fields in the UI DB")
        .def(nb::init())
        .def_rw("array_length", &ArrayField::arrayLength)
        .def("__repr__", [](const ArrayField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("ArrayField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, array_length={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.arrayLength);
        });

    nb::class_<FieldArrayField, BaseField>(m, "FieldArrayField", "Struct containing elements of field array fields in the UI DB")
        .def(nb::init())
        .def_rw("array_length", &FieldArrayField::arrayLength)
        .def_rw("field_size", &FieldArrayField::fieldSize)
        .def_rw("fields", &FieldArrayField::fields, nb::rv_policy::reference_internal)
        .def("__repr__", [](const FieldArrayField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("FieldArrayField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, array_length={!r}, field_size={!r}, "
                           "fields={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.arrayLength, field.fieldSize,
                        field.fields);
        });

    nb::class_<MessageDefinition>(m, "MessageDefinition", "Struct containing elements of message definitions in the UI DB")
        .def(nb::init())
        .def_rw("id", &MessageDefinition::_id)
        .def_rw("log_id", &MessageDefinition::logID)
        .def_rw("name", &MessageDefinition::name)
        .def_rw("description", &MessageDefinition::description)
        .def_prop_ro("fields",
                     [](MessageDefinition& self) {
                         nb::dict py_map;
                         for (const auto& [id, value] : self.fields) { py_map[nb::cast(id)] = nb::cast(value); }
                         return py_map;
                     })
        .def_rw("latest_message_crc", &MessageDefinition::latestMessageCrc)
        .def("__repr__", [](nb::handle_t<MessageDefinition> self) {
            auto& msg_def = nb::cast<MessageDefinition&>(self);
            return nb::str("MessageDefinition(name={!r}, id={!r}, log_id={!r}, description={!r}, fields={!r}, latest_message_crc={!r})")
                .format(msg_def.name, msg_def._id, msg_def.logID, msg_def.description, self.attr("fields"), msg_def.latestMessageCrc);
        });

    nb::class_<PyMessageDatabase>(m, "MessageDatabase")
        .def(nb::init())
        .def(nb::init<std::vector<MessageDefinition::ConstPtr>, std::vector<EnumDefinition::ConstPtr>>(), "msg_defs"_a, "enum_defs"_a)
        .def(nb::init<PyMessageDatabase>(), "other_db"_a)
        .def("merge", &PyMessageDatabase::Merge, "other_db"_a)
        .def("append_messages", &PyMessageDatabase::AppendJsonDbMessages, "file_path"_a)
        .def("append_enumerations", &PyMessageDatabase::AppendJsonDbEnumerations, "file_path"_a)
        .def("remove_message", &PyMessageDatabase::RemoveMessage, "msg_id"_a)
        .def("remove_enumeration", &PyMessageDatabase::RemoveEnumeration, "enumeration"_a)
        .def("get_msg_def", nb::overload_cast<const std::string&>(&PyMessageDatabase::GetMsgDef, nb::const_), "msg_name"_a)
        .def("get_msg_def", nb::overload_cast<int32_t>(&PyMessageDatabase::GetMsgDef, nb::const_), "msg_id"_a)
        .def("get_enum_def", &PyMessageDatabase::GetEnumDefId, "enum_id"_a)
        .def("get_enum_def", &PyMessageDatabase::GetEnumDefName, "enum_name"_a)
        .def_prop_ro("enums", &PyMessageDatabase::GetEnumsByNameDict)
        .def_prop_ro("messages", &PyMessageDatabase::GetMessagesByNameDict);
}

PyMessageDatabase::PyMessageDatabase()
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PyMessageDatabase::PyMessageDatabase(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_,
                                     std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_)
    : MessageDatabase(std::move(vMessageDefinitions_), std::move(vEnumDefinitions_))
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PyMessageDatabase::PyMessageDatabase(const MessageDatabase& message_db) noexcept : MessageDatabase(message_db)
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PyMessageDatabase::PyMessageDatabase(const MessageDatabase&& message_db) noexcept : MessageDatabase(message_db)
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

void PyMessageDatabase::GenerateMessageMappings()
{
    MessageDatabase::GenerateMessageMappings();
    UpdatePythonMessageTypes();
}

void PyMessageDatabase::GenerateEnumMappings()
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

inline void PyMessageDatabase::UpdatePythonEnums()
{
    nb::object IntEnum = nb::module_::import_("enum").attr("IntEnum");
    enums_by_id.clear();
    enums_by_name.clear();
    for (const auto& enum_def : EnumDefinitions())
    {
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

void PyMessageDatabase::AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, nb::handle type_constructor,
                                     nb::handle type_tuple, nb::handle type_dict)
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
            AddFieldType(field_array_field->fields, field_name, type_constructor, type_tuple, type_dict);
        }
    }
}

void PyMessageDatabase::UpdatePythonMessageTypes()
{
    // clear existing definitions
    messages_by_name.clear();

    // get type constructor
    nb::object type_constructor = nb::module_::import_("builtins").attr("type");
    // specify the python superclasses for the new message and message body types
    nb::tuple message_type_tuple = nb::make_tuple(nb::type<oem::PyMessage>());
    nb::tuple field_type_tuple = nb::make_tuple(nb::type<oem::PyField>());
    // provide no additional attributes via `__dict__`
    nb::dict type_dict = nb::dict();

    // add message and message body types for each message definition
    for (const auto& message_def : MessageDefinitions())
    {
        uint32_t crc = message_def->latestMessageCrc;
        nb::object msg_type_def = type_constructor(message_def->name, message_type_tuple, type_dict);
        messages_by_name[message_def->name] = new PyMessageType(msg_type_def, crc);
        // add additional MessageBody types for each field array element within the message definition
        AddFieldType(message_def->fields.at(crc), message_def->name, type_constructor, field_type_tuple, type_dict);
    }
}
