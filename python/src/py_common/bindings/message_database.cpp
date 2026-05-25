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

// Register custom garbage collection logic for message database so that
// it can share a lifetime with its owned types
// https://nanobind.readthedocs.io/en/latest/refleaks.html#fixing-reference-leaks

int py_common::db_tp_traverse(PyObject* self, visitproc visit, void* arg)
{
    // We must traverse the implicit dependency of an object on its
    // associated type object.
    Py_VISIT(Py_TYPE(self));

    // The tp_traverse method may be called after __new__ but before or during
    // __init__, before the C++ constructor has been completed. We must not
    // inspect the C++ state if the constructor has not yet completed.
    if (!nb::inst_ready(self)) { return 0; }

    // Get the C++ object associated with 'self' (this always succeeds)
    py_common::PyMessageDatabase* db = nb::inst_ptr<py_common::PyMessageDatabase>(self);

    // Traverse all python types held by the database
    for (auto& message_version_types : db->messages_types)
    {
        for (auto& message_type : message_version_types.second) { Py_VISIT(message_type.second.ptr()); }
    }
    for (auto& field_type : db->field_types) { Py_VISIT(field_type.second.ptr()); }
    for (auto& enum_type : db->enum_types) { Py_VISIT(enum_type.second.ptr()); }

    return 0;
}

int py_common::db_tp_clear(PyObject* self)
{
    // Get the C++ object associated with 'self' (this always succeeds)
    py_common::PyMessageDatabase* db = nb::inst_ptr<py_common::PyMessageDatabase>(self);

    // Break the reference cycle!
    db->messages_types.clear();
    db->field_types.clear();
    db->enum_types.clear();
    db->message_type_lookup_.clear();
    db->field_type_lookup_.clear();
    db->enum_type_lookup_.clear();

    return 0;
}

// Table of custom type slots we want to install
PyType_Slot db_slots[] = {{Py_tp_traverse, (void*)py_common::db_tp_traverse}, {Py_tp_clear, (void*)py_common::db_tp_clear}, {0, 0}};

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
        .def(nb::init<uint32_t, std::string, std::string>(), "value"_a, "name"_a, "description"_a)
        .def_rw("value", &EnumDataType::value)
        .def_rw("name", &EnumDataType::name)
        .def_rw("description", &EnumDataType::description)
        .def("__eq__", [](const EnumDataType& self, const EnumDataType& other) { return self == other; })
        .def("__repr__", [](const EnumDataType& enum_data_type) {
            if (enum_data_type.description.empty())
                return nb::str("EnumDataType(name={!r}, value={!r})").format(enum_data_type.name, enum_data_type.value);
            return nb::str("EnumDataType(name={!r}, value={!r}, description={!r})")
                .format(enum_data_type.name, enum_data_type.value, enum_data_type.description);
        });

    nb::class_<EnumDefinition>(m, "EnumDefinition", "Enum Definition representing contents of UI DB")
        .def(nb::init())
        .def(
            "__init__",
            [](EnumDefinition* t, std::string id, std::string name, std::vector<EnumDataType> enumerators) {
                auto* def = new (t) EnumDefinition; // NOLINT(*.NewDeleteLeaks)
                def->_id = std::move(id);
                def->name = std::move(name);
                def->enumerators = std::move(enumerators);
                for (const auto& enumerator : def->enumerators)
                {
                    def->nameValue[enumerator.name] = enumerator.value;
                    def->valueName[enumerator.value] = enumerator.name;
                    def->descriptionValue[enumerator.description] = enumerator.value;
                }
            },
            "id"_a = std::string{}, "name"_a = std::string{}, "enumerators"_a = std::vector<EnumDataType>{})
        .def_rw("id", &EnumDefinition::_id)
        .def_rw("name", &EnumDefinition::name)
        .def_rw("enumerators", &EnumDefinition::enumerators)
        .def("__repr__", [](const EnumDefinition& enum_def) {
            return nb::str("EnumDefinition(id={!r}, name={!r}, enumerators={!r})").format(enum_def._id, enum_def.name, enum_def.enumerators);
        });

    nb::class_<BaseField>(m, "FieldDefinition", "Struct containing elements of basic fields in the UI DB")
        .def(nb::init())
        .def(
            "__init__",
            [](BaseField* t, std::string name, FIELD_TYPE type, std::string conversion, DATA_TYPE data_type) {
                auto* field = new (t) BaseField; // NOLINT(*.NewDeleteLeaks)
                field->name = std::move(name);
                field->type = type;
                field->dataType.name = data_type;
                field->dataType.length = static_cast<uint16_t>(DataTypeSize(data_type));
                if (!conversion.empty()) { field->SetConversion(std::move(conversion)); }
            },
            "name"_a = std::string{}, "type"_a = FIELD_TYPE::UNKNOWN, "conversion"_a = std::string{}, "data_type"_a = DATA_TYPE::UNKNOWN)
        .def_rw("name", &BaseField::name)
        .def_rw("type", &BaseField::type)
        .def_rw("description", &BaseField::description)
        .def_prop_rw(
            "conversion", [](const BaseField& self) -> const std::string& { return self.conversion; },
            [](BaseField& self, std::string value) {
                try
                {
                    self.SetConversion(std::move(value));
                }
                catch (const std::exception& e)
                {
                    throw nb::attribute_error(e.what());
                }
            })
        .def_prop_rw(
            "data_type", [](const BaseField& self) { return self.dataType.name; },
            [](BaseField& self, DATA_TYPE value) {
                self.dataType.name = value;
                self.dataType.length = static_cast<uint16_t>(DataTypeSize(value));
            })
        .def("set_conversion", &BaseField::SetConversion, "conversion"_a)
        .def("__eq__", [](const BaseField& self, const BaseField& other) { return self == other; })
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
        .def(
            "__init__",
            [](EnumField* t, std::string name, FIELD_TYPE type, std::string conversion, DATA_TYPE data_type, std::string enum_id,
               std::vector<EnumDataType> enumerators) {
                auto* field = new (t) EnumField; // NOLINT(*.NewDeleteLeaks)
                field->name = std::move(name);
                field->type = type;
                field->dataType.name = data_type;
                field->dataType.length = static_cast<uint16_t>(DataTypeSize(data_type));
                if (!conversion.empty()) { field->SetConversion(std::move(conversion)); }
                field->enumId = std::move(enum_id);
                if (!enumerators.empty())
                {
                    auto enum_def = std::make_shared<EnumDefinition>();
                    enum_def->enumerators = std::move(enumerators);
                    for (const auto& enumerator : enum_def->enumerators)
                    {
                        enum_def->nameValue[enumerator.name] = enumerator.value;
                        enum_def->valueName[enumerator.value] = enumerator.name;
                        enum_def->descriptionValue[enumerator.description] = enumerator.value;
                    }
                    field->enumDef = std::move(enum_def);
                }
            },
            "name"_a = std::string{}, "type"_a = FIELD_TYPE::ENUM, "conversion"_a = std::string{}, "data_type"_a = DATA_TYPE::UNKNOWN,
            "enum_id"_a = std::string{}, "enumerators"_a = std::vector<EnumDataType>{})
        .def_rw("enum_id", &EnumField::enumId)
        .def_ro("enum_def", &EnumField::enumDef)
        .def_prop_rw(
            "enumerators",
            [](const EnumField& self) { return self.enumDef ? self.enumDef->enumerators : std::vector<EnumDataType>{}; },
            [](EnumField& self, std::vector<EnumDataType> enumerators) {
                auto new_def = std::make_shared<EnumDefinition>();
                if (self.enumDef)
                {
                    new_def->_id = self.enumDef->_id;
                    new_def->name = self.enumDef->name;
                }
                new_def->enumerators = std::move(enumerators);
                for (const auto& enumerator : new_def->enumerators)
                {
                    new_def->nameValue[enumerator.name] = enumerator.value;
                    new_def->valueName[enumerator.value] = enumerator.name;
                    new_def->descriptionValue[enumerator.description] = enumerator.value;
                }
                self.enumDef = std::move(new_def);
            })
        .def("__repr__", [](const EnumField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("EnumField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, enum_id={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.enumId);
        });

    nb::class_<ArrayField, BaseField>(m, "ArrayFieldDefinition", "Struct containing elements of array fields in the UI DB")
        .def(nb::init())
        .def(
            "__init__",
            [](ArrayField* t, std::string name, FIELD_TYPE type, std::string conversion, DATA_TYPE data_type, uint32_t array_length) {
                auto* field = new (t) ArrayField; // NOLINT(*.NewDeleteLeaks)
                field->name = std::move(name);
                field->type = type;
                field->dataType.name = data_type;
                field->dataType.length = static_cast<uint16_t>(DataTypeSize(data_type));
                if (!conversion.empty()) { field->SetConversion(std::move(conversion)); }
                field->arrayLength = array_length;
            },
            "name"_a = std::string{}, "type"_a = FIELD_TYPE::UNKNOWN, "conversion"_a = std::string{}, "data_type"_a = DATA_TYPE::UNKNOWN,
            "array_length"_a = uint32_t{0})
        .def_rw("array_length", &ArrayField::arrayLength)
        .def("__repr__", [](const ArrayField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("ArrayField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, array_length={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.arrayLength);
        });

    nb::class_<FieldArrayField, BaseField>(m, "FieldArrayFieldDefinition", "Struct containing elements of field array fields in the UI DB")
        .def(
            "__init__",
            [](FieldArrayField* t, std::string name, FIELD_TYPE type, std::string conversion, DATA_TYPE data_type, uint32_t array_length,
               std::vector<std::shared_ptr<BaseField>> fields) {
                auto* field = new (t) FieldArrayField; // NOLINT(*.NewDeleteLeaks)
                field->name = std::move(name);
                field->type = type;
                field->dataType.name = data_type;
                field->dataType.length = static_cast<uint16_t>(DataTypeSize(data_type));
                if (!conversion.empty()) { field->SetConversion(std::move(conversion)); }
                field->arrayLength = array_length;
                field->fields = std::move(fields);
            },
            "name"_a = std::string{}, "type"_a = FIELD_TYPE::FIELD_ARRAY, "conversion"_a = std::string{}, "data_type"_a = DATA_TYPE::UNKNOWN,
            "array_length"_a = uint32_t{0}, "fields"_a = std::vector<std::shared_ptr<BaseField>>{})
        .def_rw("array_length", &FieldArrayField::arrayLength)
        .def_rw("fields", &FieldArrayField::fields, nb::rv_policy::reference_internal)
        .def("__repr__", [](const FieldArrayField& field) {
            const std::string& desc = field.description == "[Brief Description]" ? "" : field.description;
            return nb::str("FieldArrayField(name={!r}, type={}, data_type={}, description={!r}, conversion={!r}, array_length={!r}, "
                           "fields={!r})")
                .format(field.name, nb::cast(field.type), field.dataType.name, desc, field.conversion, field.arrayLength, field.fields);
        });

    nb::class_<MessageDefinition>(m, "MessageDefinition", "Struct containing elements of message definitions in the UI DB")
        .def(nb::init())
        .def(
            "__init__",
            [](MessageDefinition* t, std::string id, uint32_t log_id, std::string name, std::string description, uint32_t latest_message_crc,
               std::unordered_map<uint32_t, std::vector<std::shared_ptr<BaseField>>> fields) {
                auto* def = new (t) MessageDefinition; // NOLINT(*.NewDeleteLeaks)
                def->_id = std::move(id);
                def->logID = log_id;
                def->name = std::move(name);
                def->description = std::move(description);
                def->latestMessageCrc = latest_message_crc;
                def->fields = std::move(fields);
            },
            "id"_a = std::string{}, "log_id"_a = uint32_t{0}, "name"_a = std::string{}, "description"_a = std::string{},
            "latest_message_crc"_a = uint32_t{0},
            "fields"_a = std::unordered_map<uint32_t, std::vector<std::shared_ptr<BaseField>>>{})
        .def_rw("id", &MessageDefinition::_id)
        .def_rw("log_id", &MessageDefinition::logID)
        .def_rw("name", &MessageDefinition::name)
        .def_rw("description", &MessageDefinition::description)
        .def_prop_rw(
            "fields",
            [](MessageDefinition& self) {
                nb::dict py_map;
                for (const auto& [id, value] : self.fields) { py_map[nb::cast(id)] = nb::cast(value); }
                return py_map;
            },
            [](MessageDefinition& self, std::unordered_map<uint32_t, std::vector<std::shared_ptr<BaseField>>> fields) {
                self.fields = std::move(fields);
            })
        .def_rw("latest_message_crc", &MessageDefinition::latestMessageCrc)
        .def("__eq__", [](const MessageDefinition& self, const MessageDefinition& other) { return self == other; })
        .def("__repr__", [](nb::handle_t<MessageDefinition> self) {
            auto& msg_def = nb::cast<MessageDefinition&>(self);
            return nb::str("MessageDefinition(name={!r}, id={!r}, log_id={!r}, description={!r}, fields={!r}, latest_message_crc={!r})")
                .format(msg_def.name, msg_def._id, msg_def.logID, msg_def.description, self.attr("fields"), msg_def.latestMessageCrc);
        });

    nb::class_<py_common::PyMessageDatabase>(m, "MessageDatabase", nb::type_slots(db_slots))
        .def(nb::new_([]() { return py_common::PyMessageDatabase::Create(); }))
        .def(nb::new_([](std::filesystem::path& file_path) { return py_common::PyMessageDatabase::Create(std::move(*LoadJsonDbFile(file_path))); }),
             "file_path"_a)
        .def_static(
            "from_string", [](std::string_view json_data) { return py_common::PyMessageDatabase::Create(std::move(*ParseJsonDb(json_data))); },
            "json_data"_a)
        .def("merge", &py_common::PyMessageDatabase::Merge, "other_db"_a)
        .def("append_messages", &py_common::PyMessageDatabase::AppendMessages, "messages"_a)
        .def("append_enumerations", &py_common::PyMessageDatabase::AppendEnumerations, "enums"_a)
        .def("remove_message", &py_common::PyMessageDatabase::RemoveMessage, "msg_id"_a)
        .def("remove_enumeration", &py_common::PyMessageDatabase::RemoveEnumeration, "enumeration"_a)
        .def(
            "get_msg_def",
            [](const py_common::PyMessageDatabase& self, std::string_view msg_name) -> MessageDefinition::ConstPtr {
                auto def = self.GetMsgDef(msg_name);
                return def ? std::make_shared<MessageDefinition>(*def) : nullptr;
            },
            "msg_name"_a)
        .def(
            "get_msg_def",
            [](const py_common::PyMessageDatabase& self, int32_t msg_id) -> MessageDefinition::ConstPtr {
                auto def = self.GetMsgDef(msg_id);
                return def ? std::make_shared<MessageDefinition>(*def) : nullptr;
            },
            "msg_id"_a)
        .def("get_enum_def", &py_common::PyMessageDatabase::GetEnumDefId, "enum_id"_a)
        .def("get_enum_def_by_id", &py_common::PyMessageDatabase::GetEnumDefId, "enum_id"_a)
        .def("get_enum_def_by_name", &py_common::PyMessageDatabase::GetEnumDefName, "enum_name"_a)
        .def(
            "get_msg_type", [](py_common::PyMessageDatabase& self, std::string name) { return self.GetMessageType(name); }, "name"_a)
        .def(
            "get_enum_type_by_name", [](py_common::PyMessageDatabase& self, std::string name) { return self.GetEnumTypeByName(name); }, "name"_a)
        .def(
            "get_enum_type_by_id", [](py_common::PyMessageDatabase& self, std::string id) { return self.GetEnumTypeById(id); }, "id"_a)
        .def_prop_rw("message_family", &py_common::PyMessageDatabase::GetMessageFamily, &py_common::PyMessageDatabase::SetMessageFamily)
        .def("clone", &py_common::PyMessageDatabase::clone, "Create a copy of this database.");
}
