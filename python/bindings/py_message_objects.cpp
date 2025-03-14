#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "exceptions.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"
#include "encoder.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;

NB_MAKE_OPAQUE(std::vector<FieldContainer>);

#pragma region PyHeader Methods

nb::dict PyHeader::to_dict() const
{
    nb::dict header_dict;
    header_dict["message_id"] = usMessageId;
    header_dict["message_type"] = ucMessageType;
    header_dict["port_address"] = uiPortAddress;
    header_dict["length"] = usLength;
    header_dict["sequence"] = usSequence;
    header_dict["idle_time"] = ucIdleTime;
    header_dict["time_status"] = uiTimeStatus;
    header_dict["week"] = usWeek;
    header_dict["milliseconds"] = dMilliseconds;
    header_dict["receiver_status"] = uiReceiverStatus;
    header_dict["message_definition_crc"] = uiMessageDefinitionCrc;
    header_dict["receiver_sw_version"] = usReceiverSwVersion;
    return header_dict;
}

#pragma endregion

#pragma region PyField Methods

nb::object convert_field(const FieldContainer& field, const PyMessageDatabase::ConstPtr& parent_db, std::string parent, bool has_ptype)
{
    if (field.fieldDef->type == FIELD_TYPE::ENUM)
    {
        // Handle Enums
        const std::string& enumId = static_cast<const EnumField*>(field.fieldDef.get())->enumId;
        auto it = parent_db->GetEnumsByIdDict().find(enumId);
        if (it == parent_db->GetEnumsByIdDict().end())
        {
            throw std::runtime_error("Enum definition for " + field.fieldDef->name + " field with ID '" + enumId +
                                     "' not found in the JSON database");
        }
        nb::object enum_type = it->second;
        return std::visit([&](auto&& value) { return enum_type(value); }, field.fieldValue);
    }
    else if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
    {
        const auto& message_field = std::get<std::vector<FieldContainer>>(field.fieldValue);
        if (message_field.empty())
        {
            // Handle Empty Arrays
            return nb::list();
        }
        else if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
        {
            // Handle Field Arrays
            nb::handle field_ptype;
            std::string field_name;
            if (has_ptype)
            {
                // If a parent type name is provided, get a field type based on the parent and field name
                field_name = parent + "_" + field.fieldDef->name + "_Field";
                try
                {
                    field_ptype = parent_db->GetFieldsByNameDict().at(field_name);
                }
                catch (const std::out_of_range& e)
                {
                    // This case should never happen, if it does there is a bug
                    throw std::runtime_error("Field type not found for " + field_name);
                }
            }
            else
            {
                // If field has no ptype, use the generic "Field" type
                field_name = std::move(parent);
                field_ptype = nb::type<PyField>();
            }

            // Create an appropriate PyField instance for each subfield in the array
            std::vector<nb::object> sub_values;
            sub_values.reserve(message_field.size());
            for (const auto& subfield : message_field)
            {
                nb::object pyinst = nb::inst_alloc(field_ptype);
                PyField* cinst = nb::inst_ptr<PyField>(pyinst);
                const auto& message_subfield = std::get<std::vector<FieldContainer>>(subfield.fieldValue);
                new (cinst) PyField(field_name, has_ptype, message_subfield, parent_db);
                nb::inst_mark_ready(pyinst);
                sub_values.push_back(pyinst);
            }
            return nb::cast(sub_values);
        }
        else
        {
            // Handle Fixed or Variable-Length Arrays
            if (field.fieldDef->conversion == "%s")
            {
                // The array is actually a string
                std::string str;
                str.reserve(message_field.size());
                for (const auto& sub_field : message_field)
                {
                    auto c = std::get<uint8_t>(sub_field.fieldValue);
                    if (c == 0) { break; }
                    str.push_back(c);
                }
                return nb::cast(str);
            }
            std::vector<nb::object> sub_values;
            sub_values.reserve(message_field.size());
            for (const auto& f : message_field) { sub_values.push_back(convert_field(f, parent_db, parent, has_ptype)); }
            return nb::cast(sub_values);
        }
    }
    else if (field.fieldDef->conversion == "%id")
    {
        // Handle Satellite IDs
        const uint32_t temp_id = std::get<uint32_t>(field.fieldValue);
        SatelliteId sat_id;
        sat_id.usPrnOrSlot = temp_id & 0x0000FFFF;
        sat_id.sFrequencyChannel = (temp_id & 0xFFFF0000) >> 16;
        return nb::cast(sat_id);
    }
    else
    {
        // Handle most types by simply extracting the value from the variant and casting
        return std::visit([](auto&& value) { return nb::cast(value); }, field.fieldValue);
    }
}

nb::dict& PyField::get_values() const
{
    if (cached_values_.size() == 0)
    {
        for (const auto& field : fields)
        {
            cached_values_[nb::cast(field.fieldDef->name)] = convert_field(field, parent_db_, this->name, this->has_ptype);
        }
    }
    return cached_values_;
}

nb::dict& PyField::get_fields() const
{
    if (cached_fields_.size() == 0)
    {
        for (const auto& field : fields) { cached_fields_[nb::cast(field.fieldDef->name)] = field.fieldDef; }
    }
    return cached_fields_;
}

nb::dict PyField::to_dict() const
{
    nb::dict dict;
    for (const auto& [field_name, value] : get_values())
    {
        if (nb::isinstance<PyField>(value)) { dict[field_name] = nb::cast<PyField>(value).to_dict(); }
        else if (nb::isinstance<std::vector<nb::object>>(value))
        {
            nb::list list;
            for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value))
            {
                if (nb::isinstance<PyField>(sub_item)) { list.append(nb::cast<PyField>(sub_item).to_dict()); }
                else { list.append(sub_item); }
            }
            dict[field_name] = list;
        }
        else { dict[field_name] = value; }
    }
    return dict;
}

nb::object PyField::getattr(nb::str field_name) const
{
    if (!contains(field_name)) { throw nb::attribute_error(field_name.c_str()); }
    return get_values()[std::move(field_name)];
}

nb::object PyField::getitem(nb::str field_name) const { return get_values()[std::move(field_name)]; }

bool PyField::contains(nb::str field_name) const { return get_values().contains(std::move(field_name)); }

size_t PyField::len() const { return fields.size(); }

std::string PyField::repr() const
{
    std::stringstream repr;
    repr << name << "(";
    bool first = true;
    for (const auto& [field_name, value] : get_values())
    {
        if (!first) { repr << ", "; }
        first = false;
        repr << nb::str("{}={!r}").format(field_name, value).c_str();
    }
    repr << ")";
    return repr.str();
}

#pragma endregion

#pragma region PyMessageMethods

PyMessageData PyMessage::to_ascii() { 
    PyMessageDatabase::ConstPtr db = this->parent_db_;
    std::shared_ptr<const PyEncoder> encoder = std::static_pointer_cast<const PyEncoder>(db->get_encoder());
    return encoder->PyEncode(*this, ENCODE_FORMAT::ASCII);
}

PyMessageData PyMessage::to_binary()
{
    PyMessageDatabase::ConstPtr db = this->parent_db_;
    std::shared_ptr<const PyEncoder> encoder = std::static_pointer_cast<const PyEncoder>(db->get_encoder());
    return encoder->PyEncode(*this, ENCODE_FORMAT::BINARY);
}

PyMessageData PyMessage::to_flattended_binary()
{
    PyMessageDatabase::ConstPtr db = this->parent_db_;
    std::shared_ptr<const PyEncoder> encoder = std::static_pointer_cast<const PyEncoder>(db->get_encoder());
    return encoder->PyEncode(*this, ENCODE_FORMAT::FLATTENED_BINARY);
}

PyMessageData PyMessage::to_json()
{
    PyMessageDatabase::ConstPtr db = this->parent_db_;
    std::shared_ptr<const PyEncoder> encoder = std::static_pointer_cast<const PyEncoder>(db->get_encoder());
    return encoder->PyEncode(*this, ENCODE_FORMAT::JSON);
}


#pragma endregion

#pragma region Message Constructor Functions

nb::object oem::create_unknown_bytes(nb::bytes data) {
    nb::handle data_pytype = nb::type<PyUnknownBytes>();
    nb::object data_pyinst = nb::inst_alloc(data_pytype);
    PyUnknownBytes* data_cinst = nb::inst_ptr<PyUnknownBytes>(data_pyinst);
    new (data_cinst) PyUnknownBytes(data); 
    nb::inst_mark_ready(data_pyinst);
    return data_pyinst;
}

nb::object oem::create_unknown_message_instance(nb::bytes data, PyHeader& header, PyMessageDatabase::ConstPtr database)
{
    nb::handle message_pytype = nb::type<PyUnknownMessage>();
    nb::object message_pyinst = nb::inst_alloc(message_pytype);
    PyUnknownMessage* message_cinst = nb::inst_ptr<PyUnknownMessage>(message_pyinst);
    new (message_cinst) PyUnknownMessage(database, header, data);
    nb::inst_mark_ready(message_pyinst);
    return message_pyinst;
}

nb::object oem::create_message_instance(PyHeader& header, std::vector<FieldContainer>& message_fields, MetaDataStruct& metadata, PyMessageDatabase::ConstPtr database)
{
    nb::handle message_pytype;

    const std::string message_name = metadata.messageName;

    bool has_ptype = true;

    try
    {
        PyMessageType* message_type_struct = database->GetMessagesByNameDict().at(message_name);
        if (message_type_struct->crc == metadata.uiMessageCrc)
        {
            // If the CRCs match, use the specific message type
            message_pytype = message_type_struct->python_type;
        }
        else
        {
            // If the CRCs don't match, use the generic "CompleteMessage" type
            message_pytype = nb::type<PyMessage>();
            has_ptype = false;
        }
    }
    catch (const std::out_of_range& e)
    {
        // This case should never happen, if it does there is a bug
        throw std::runtime_error("Message name '" + message_name + "' not found in the JSON database");
    }
    nb::object message_pyinst = nb::inst_alloc(message_pytype);
    PyMessage* message_cinst = nb::inst_ptr<PyMessage>(message_pyinst);
    new (message_cinst) PyMessage(message_name, has_ptype, message_fields, database, header);

    nb::inst_mark_ready(message_pyinst);
    return message_pyinst;
}

#pragma endregion

#pragma region Bindings

void init_header_objects(nb::module_& m) {
    nb::class_<oem::PyMessageTypeField>(m, "MessageType")
        .def("__repr__",
             [](nb::handle_t<PyMessageTypeField> self) {
                 std::string source = nb::cast<nb::str>(self.attr("source").attr("__repr__")()).c_str();
                 std::string format = nb::cast<nb::str>(self.attr("format").attr("__repr__")()).c_str();
                 std::string response = nb::cast<nb::str>(self.attr("is_response").attr("__repr__")()).c_str();

                 return "MessageType(source=" + source + ", format=" + format + ", response=" + response + ")";
             })
        .def_prop_ro("is_response", &oem::PyMessageTypeField::IsResponse)
        .def_prop_ro("format", &oem::PyMessageTypeField::GetFormat)
        .def_prop_ro("source", &oem::PyMessageTypeField::GetMeasurementSource);

    nb::class_<oem::PyHeader>(m, "Header")
        .def_ro("message_id", &oem::PyHeader::usMessageId)
        .def_prop_ro("message_type", &oem::PyHeader::GetPyMessageType)
        .def_ro("port_address", &oem::PyHeader::uiPortAddress)
        .def_ro("length", &oem::PyHeader::usLength)
        .def_ro("sequence", &oem::PyHeader::usSequence)
        .def_ro("idle_time", &oem::PyHeader::ucIdleTime)
        .def_ro("time_status", &oem::PyHeader::uiTimeStatus)
        .def_ro("week", &oem::PyHeader::usWeek)
        .def_ro("milliseconds", &oem::PyHeader::dMilliseconds)
        .def_ro("receiver_status", &oem::PyHeader::uiReceiverStatus)
        .def_ro("message_definition_crc", &oem::PyHeader::uiMessageDefinitionCrc)
        .def_ro("receiver_sw_version", &oem::PyHeader::usReceiverSwVersion)
        .def("to_dict", [](const oem::PyHeader& self) { return self.to_dict(); })
        .def("__repr__", [](const nb::handle self) {
            auto& header = nb::cast<oem::PyHeader&>(self);
            return nb::str("Header(message_id={!r}, message_type={!r}, port_address={!r}, length={!r}, sequence={!r}, "
                           "idle_time={!r}, time_status={!r}, week={!r}, milliseconds={!r}, receiver_status={!r}, "
                           "message_definition_crc={!r}, receiver_sw_version={!r})")
                .format(header.usMessageId, header.ucMessageType, header.uiPortAddress, header.usLength, header.usSequence, header.ucIdleTime,
                        header.uiTimeStatus, header.usWeek, header.dMilliseconds, header.uiReceiverStatus, header.uiMessageDefinitionCrc,
                        header.usReceiverSwVersion);
        });
}

void init_message_objects(nb::module_& m) {

    nb::class_<PyEdieData>(m, "EdieData");

    nb::class_<PyGpsTime, PyEdieData>(m, "GpsTime")
        .def(nb::init())
        .def(nb::init<uint16_t, double, TIME_STATUS>(), "week"_a, "milliseconds"_a, "time_status"_a = TIME_STATUS::UNKNOWN)
        .def_rw("week", &PyGpsTime::week)
        .def_rw("milliseconds", &PyGpsTime::milliseconds)
        .def_rw("status", &PyGpsTime::time_status);

    nb::class_<PyUnknownBytes, PyEdieData>(m, "UnknownBytes")
        .def("__repr__",
             [](const PyUnknownBytes self) {
                 std::string byte_rep = nb::str(self.data.attr("__repr__")()).c_str();
                 return "UnknownBytes(" + byte_rep + ")";
             })
        .def_ro("data", &PyUnknownBytes::data);

    nb::class_<PyField, PyEdieData>(m, "Field")
        .def("to_dict", &PyField::to_dict, "Convert the message and its sub-messages into a dict")
        .def("__getattr__", &PyField::getattr, "field_name"_a)
        .def("__repr__", &PyField::repr)
        .def("__dir__", [](nb::object self) {
            // get required Python builtin functions
            nb::module_ builtins = nb::module_::import_("builtins");
            nb::handle super = builtins.attr("super");
            nb::handle type = builtins.attr("type");

            // start from the 'Field' class instead of a specific subclass
            nb::handle current_type = type(self);
            std::string current_type_name = nb::cast<std::string>(current_type.attr("__name__"));
            while (current_type_name != "Field")
            {
                current_type = (current_type.attr("__bases__"))[0];
                current_type_name = nb::cast<std::string>(current_type.attr("__name__"));
            }

            // retrieve base list based on 'Field' superclass method
            nb::object super_obj = super(current_type, self);
            nb::list base_list = nb::cast<nb::list>(super_obj.attr("__dir__")());
            // add dynamic fields to the list
            PyField* body = nb::inst_ptr<PyField>(self);
            for (const auto& [field_name, _] : body->get_fields()) { base_list.append(field_name); }

            return base_list;
        });

    nb::class_<PyMessageBase, PyField>(m, "MessageBase")
        .def_ro("header", &PyMessageBase::header)
        .def(
            "to_dict",
            [](const PyMessageBase& self, bool include_header) {
                nb::dict dict = self.to_dict();
                if (include_header) { dict["header"] = self.header.to_dict(); }
                return dict;
            },
            "include_header"_a = true, "Convert the message and its sub-messages into a dict");

    nb::class_<PyUnknownMessage, PyMessageBase>(m, "UnknownMessage")
        .def("__repr__",
             [](const PyUnknownMessage self) {
                 std::string byte_rep = nb::str(self.payload.attr("__repr__")()).c_str();
                 return "IncompleteMessage(payload=" + byte_rep + ")";
             })
        .def_ro("bytes", &PyUnknownMessage::payload);

    nb::class_<PyMessage, PyMessageBase>(m, "Message")
        .def("to_ascii", &PyMessage::to_ascii)
        .def("to_binary", &PyMessage::to_binary)
        .def("to_flattended_binary", &PyMessage::to_flattended_binary)
        .def("to_json", &PyMessage::to_json)
        .def_ro("name", &PyMessage::name);
}

#pragma endregion
