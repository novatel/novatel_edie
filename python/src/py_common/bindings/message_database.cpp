#include "py_common/message_database.hpp"

#include <nanobind/stl/unordered_map.h>

#include "novatel_edie/decoders/common/json_db_reader.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/field_objects.hpp"
#include "py_common/init_bindings.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_common::init_common_message_database(nb::module_& m)
{
    nb::enum_<DATA_TYPE>(m, "DATA_TYPE", "The concrete data types of base-level message fields.", nb::is_arithmetic())
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

    nb::enum_<FIELD_TYPE>(m, "FIELD_TYPE", "The abstracted types of a message field.", nb::is_arithmetic())
        .value("SIMPLE", FIELD_TYPE::SIMPLE, "A value with a simple data type such as a integer or float.")
        .value("ENUM", FIELD_TYPE::ENUM, "An enum value.")
        .value("BITFIELD", FIELD_TYPE::BITFIELD, "A bitfield value.")
        .value("FIXED_LENGTH_ARRAY", FIELD_TYPE::FIXED_LENGTH_ARRAY, "An array with a pre-determined length.")
        .value("VARIABLE_LENGTH_ARRAY", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, "An array whose length length may differ across different messages.")
        .value("STRING", FIELD_TYPE::STRING, "A string value.")
        .value("FIELD_ARRAY", FIELD_TYPE::FIELD_ARRAY, "An array of complex fields with their own sub-fields.")
        .value("RESPONSE_ID", FIELD_TYPE::RESPONSE_ID, "A fabricated response ID.")
        .value("RESPONSE_STR", FIELD_TYPE::RESPONSE_STR, "A fabricated response string.")
        .value("RXCONFIG_HEADER", FIELD_TYPE::RXCONFIG_HEADER, "A fabricated RXCONFIG header.")
        .value("RXCONFIG_BODY", FIELD_TYPE::RXCONFIG_BODY, "A fabricated RXCONFIG body.")
        .value("UNKNOWN", FIELD_TYPE::UNKNOWN, "A value with an unknown type.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    m.attr("str_to_FIELD_TYPE") = FieldTypeEnumLookup;

    nb::class_<EnumDataType>(m, "EnumDataType", "Enum Data Type representing contents of UI DB")
        .def(nb::init())
        .def_rw("value", &EnumDataType::value)
        .def_rw("name", &EnumDataType::name)
        .def_rw("description", &EnumDataType::description)
        .def("__repr__", [](const EnumDataType& enum_data_type) {
            if (enum_data_type.description.empty())
                return nb::str("EnumDataType(name={!r}, value={!r})").format(enum_data_type.name, enum_data_type.value);
            return nb::str("EnumDataType(name={!r}, value={!r}, description={!r})")
                .format(enum_data_type.name, enum_data_type.value, enum_data_type.description);
        });

    nb::class_<EnumDefinition>(m, "EnummeratorDefinition", "Enum Definition representing contents of UI DB")
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

    nb::class_<BaseField>(m, "FieldDefinition", "Struct containing elements of basic fields in the UI DB")
        .def(nb::init())
        .def(nb::init<std::string, FIELD_TYPE, std::string, size_t, DATA_TYPE>(), "name"_a, "type"_a, "conversion"_a, "length"_a, "data_type"_a)
        .def_rw("name", &BaseField::name)
        .def_rw("type", &BaseField::type)
        .def_rw("description", &BaseField::description)
        .def_rw("conversion", &BaseField::conversion)
        .def_rw("width", &BaseField::width)
        .def_rw("precision", &BaseField::precision)
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

    nb::class_<EnumField, BaseField>(m, "EnumFieldDefinition", "Struct containing elements of enum fields in the UI DB")
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

    nb::class_<ArrayField, BaseField>(m, "ArrayFieldDefinition", "Struct containing elements of array fields in the UI DB")
        .def(nb::init())
        .def_rw("array_length", &ArrayField::arrayLength)
        .def("__repr__", [](const ArrayField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("ArrayField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, array_length={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.arrayLength);
        });

    nb::class_<FieldArrayField, BaseField>(m, "FieldArrayFieldDefinition", "Struct containing elements of field array fields in the UI DB")
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

    nb::class_<py_common::PyMessageDatabaseCore>(m, "MessageDatabase")
        .def(nb::new_([]() { return std::make_shared<py_common::PyMessageDatabaseCore>(); }))
        .def(nb::new_([](std::filesystem::path& file_path) {
                 return std::make_shared<py_common::PyMessageDatabaseCore>(std::move(*LoadJsonDbFile(file_path)));
             }),
             "file_path"_a)
        .def_static(
            "from_string",
            [](std::string_view json_data) { return std::make_shared<py_common::PyMessageDatabaseCore>(std::move(*ParseJsonDb(json_data))); },
            "json_data"_a)
        .def("merge", &py_common::PyMessageDatabaseCore::Merge, "other_db"_a)
        .def("append_messages", &py_common::PyMessageDatabaseCore::AppendMessages, "messages"_a)
        .def("append_enumerations", &py_common::PyMessageDatabaseCore::AppendEnumerations, "enums"_a)
        .def("remove_message", &py_common::PyMessageDatabaseCore::RemoveMessage, "msg_id"_a)
        .def("remove_enumeration", &py_common::PyMessageDatabaseCore::RemoveEnumeration, "enumeration"_a)
        .def("get_msg_def", nb::overload_cast<std::string_view>(&py_common::PyMessageDatabaseCore::GetMsgDef, nb::const_), "msg_name"_a)
        .def("get_msg_def", nb::overload_cast<int32_t>(&py_common::PyMessageDatabaseCore::GetMsgDef, nb::const_), "msg_id"_a)
        .def("get_enum_def", &py_common::PyMessageDatabaseCore::GetEnumDefId, "enum_id"_a)
        .def("get_enum_def", &py_common::PyMessageDatabaseCore::GetEnumDefName, "enum_name"_a)
        .def(
            "get_msg_type",
            [](py_common::PyMessageDatabaseCore& self, std::string name) { return self.GetMessagesByNameDict().at(name)->python_type; }, "name"_a)
        .def(
            "get_enum_type_by_name", [](py_common::PyMessageDatabaseCore& self, std::string name) { return self.GetEnumsByNameDict().at(name); },
            "name"_a)
        .def("get_enum_type_by_id", [](py_common::PyMessageDatabaseCore& self, std::string id) { return self.GetEnumsByIdDict().at(id); }, "id"_a);
}
