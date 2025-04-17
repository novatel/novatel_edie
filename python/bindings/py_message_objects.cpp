#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"

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

oem::PyMessageData PyEncode(PyMessage& py_message, const PyMessageDatabase* db, ENCODE_FORMAT format)
{
    STATUS status;
    MessageDataStruct message_data = MessageDataStruct();
    std::shared_ptr<const Encoder> encoder = db->get_encoder();

    if (format == ENCODE_FORMAT::JSON)
    {
        // Allocate more space for JSON messages.
        // A TRACKSTAT message can use about 47k bytes when encoded as JSON.
        // FIXME: this is still not safe and there is no effective buffer overflow checking implemented in Encoder.
        uint8_t buffer[MESSAGE_SIZE_MAX * 3];
        auto* buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
        uint32_t buf_size = MESSAGE_SIZE_MAX * 3;
        status = encoder->Encode(&buf_ptr, buf_size, py_message.header, py_message.fields, message_data, py_message.header.format, format);
    }
    else
    {
        uint8_t buffer[MESSAGE_SIZE_MAX];
        auto buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
        uint32_t buf_size = MESSAGE_SIZE_MAX;
        status = encoder->Encode(&buf_ptr, buf_size, py_message.header, py_message.fields, message_data, py_message.header.format, format);
    }
    throw_exception_from_status(status);
    return PyMessageData(message_data);
}

PyMessageData PyMessage::encode(ENCODE_FORMAT fmt) { return PyEncode(*this, this->parent_db_.get(), fmt); }

PyMessageData PyMessage::to_ascii() { return PyEncode(*this, this->parent_db_.get(), ENCODE_FORMAT::ASCII); }

PyMessageData PyMessage::to_binary() { return PyEncode(*this, this->parent_db_.get(), ENCODE_FORMAT::BINARY); }

PyMessageData PyMessage::to_flattened_binary() { return PyEncode(*this, this->parent_db_.get(), ENCODE_FORMAT::BINARY); }

PyMessageData PyMessage::to_json() { return PyEncode(*this, this->parent_db_.get(), ENCODE_FORMAT::JSON); }

#pragma endregion

#pragma region Message Constructor Functions

nb::object oem::create_unknown_bytes(nb::bytes data)
{
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

nb::object oem::create_message_instance(PyHeader& header, std::vector<FieldContainer>& message_fields, MetaDataStruct& metadata,
                                        PyMessageDatabase::ConstPtr database)
{

    nb::handle message_pytype;

    const std::string message_name = metadata.messageName;

    bool has_ptype = true;

    if (metadata.bResponse)
    {

        nb::object response_pyinst = nb::inst_alloc(nb::type<PyResponse>());
        PyResponse* response_cinst = nb::inst_ptr<PyResponse>(response_pyinst);
        new (response_cinst) PyResponse(message_name, message_fields, database, header);
        nb::inst_mark_ready(response_pyinst);

        return response_pyinst;
    }
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

void init_header_objects(nb::module_& m)
{
    nb::class_<oem::PyMessageTypeField>(m, "MessageType",
                                        "A message field which provides information about its source, format, and whether it is a response.")
        .def("__repr__",
             [](nb::handle_t<PyMessageTypeField> self) {
                 std::string source = nb::cast<nb::str>(self.attr("source").attr("__repr__")()).c_str();
                 std::string format = nb::cast<nb::str>(self.attr("format").attr("__repr__")()).c_str();
                 std::string response = nb::cast<nb::str>(self.attr("is_response").attr("__repr__")()).c_str();

                 return "MessageType(source=" + source + ", format=" + format + ", response=" + response + ")";
             })
        .def_prop_ro("is_response", &oem::PyMessageTypeField::IsResponse, "Whether the message is a response.")
        .def_prop_ro("format", &oem::PyMessageTypeField::GetFormat, "The original format of the message.")
        .def_prop_ro("source", &oem::PyMessageTypeField::GetMeasurementSource, "Where the message originates from.");

    nb::class_<oem::PyHeader>(m, "Header")
        .def_ro("message_id", &oem::PyHeader::usMessageId, "The Message ID number.")
        .def_prop_ro("message_type", &oem::PyHeader::GetPyMessageType, "Information regarding the type of the message.")
        .def_ro("port_address", &oem::PyHeader::uiPortAddress, "The port the message was sent from.")
        .def_ro("length", &oem::PyHeader::usLength, "The length of the message. Will be 0 if unknown.")
        .def_ro("sequence", &oem::PyHeader::usSequence, "Number of remaning related messages following this one. Will be 0 for most messages.")
        .def_ro("idle_time", &oem::PyHeader::ucIdleTime, "Time that the processor is idle. Divide by two to get the percentage.")
        .def_ro("time_status", &oem::PyHeader::uiTimeStatus, "The quality of the GPS reference time.")
        .def_ro("week", &oem::PyHeader::usWeek, "GPS reference wekk number.")
        .def_ro("milliseconds", &oem::PyHeader::dMilliseconds, "Milliseconds from the beginning of the GPS reference week.")
        .def_ro("receiver_status", &oem::PyHeader::uiReceiverStatus,
                "32-bits representing the status of various hardware and software components of the receiver.")
        .def_ro("message_definition_crc", &oem::PyHeader::uiMessageDefinitionCrc, "A value for validating the message definition used for decoding.")
        .def_ro("receiver_sw_version", &oem::PyHeader::usReceiverSwVersion, "A value (0 - 65535) representing the receiver software build number.")
        .def(
            "to_dict", [](const oem::PyHeader& self) { return self.to_dict(); },
            R"doc(
            Converts the header to a dictionary.

            Returns:
                A dictionary representation of the header.
            )doc")
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

void init_message_objects(nb::module_& m)
{

    nb::class_<PyGpsTime>(m, "GpsTime", "A GPS reference time.")
        .def(nb::init())
        .def(nb::init<uint16_t, double>(), "week"_a, "milliseconds"_a = TIME_STATUS::UNKNOWN)
        .def(nb::init<uint16_t, double, TIME_STATUS>(), "week"_a, "milliseconds"_a, "time_status"_a = TIME_STATUS::UNKNOWN)
        .def("__repr__", [](PyGpsTime& self) { return "GPSTime(" + std::to_string(self.week) + ", " + std::to_string(self.milliseconds) + ")"; })
        .def_rw("week", &PyGpsTime::week, "GPS reference week number.")
        .def_rw("milliseconds", &PyGpsTime::milliseconds, "Milliseconds from the beginning of the GPS reference week.")
        .def_rw("status", &PyGpsTime::time_status, "The quality of the GPS reference time.");

    nb::class_<PyUnknownBytes>(m, "UnknownBytes", "A set of bytes which was determined to be undecodable by EDIE.")
        .def("__repr__",
             [](const PyUnknownBytes self) {
                 std::string byte_rep = nb::str(self.data.attr("__repr__")()).c_str();
                 return "UnknownBytes(" + byte_rep + ")";
             })
        .def_ro("data", &PyUnknownBytes::data, "The raw bytes determined to be undecodable.");

    nb::class_<PyField>(m, "Field")
        .def("to_dict", &PyField::to_dict,
             R"doc(
            Converts the field to a dictionary.

            Returns:
                A dictionary representation of the field.
            )doc")
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

    nb::class_<PyUnknownMessage>(m, "UnknownMessage")
        .def("__repr__",
             [](const PyUnknownMessage self) {
                 std::string byte_rep = nb::str(self.payload.attr("__repr__")()).c_str();
                 return "IncompleteMessage(payload=" + byte_rep + ")";
             })
        .def_ro("header", &PyUnknownMessage::header, "The header of the message.")
        .def_ro("payload", &PyUnknownMessage::payload, "The undecoded bytes that make up the message's payload.")
        .def(
            "to_dict",
            [](const PyUnknownMessage& self) {
                nb::dict dict = nb::dict();
                dict["header"] = self.header.to_dict();
                dict["payload"] = self.payload;
                return dict;
            },
            R"doc(
            Converts the message to a dictionary.

            Returns:
                A dictionary representation of the message.
            )doc");

    nb::class_<PyMessage, PyField>(m, "Message")
        .def("encode", &PyMessage::encode)
        .def("to_ascii", &PyMessage::to_ascii)
        .def("to_binary", &PyMessage::to_binary)
        .def("to_flattended_binary", &PyMessage::to_flattened_binary)
        .def("to_json", &PyMessage::to_json)
        .def(
            "to_dict",
            [](const PyMessage& self, bool include_header) {
                nb::dict dict = self.to_dict();
                if (include_header) { dict["header"] = self.header.to_dict(); }
                return dict;
            },
            "include_header"_a = true,
            R"doc(
            Converts the message to a dictionary.
            
            Args:
                include_header: Whether to include the header of the message in the 
                    new representation.

            Returns:
                A dictionary representation of the message.
            )doc")
        .def_ro("header", &PyMessage::header, "The header of the message.")
        .def_ro("name", &PyMessage::name, "The type of message it is.");

    nb::class_<PyResponse, PyField>(m, "Response")
        .def(
            "to_dict",
            [](const PyResponse& self, bool include_header) {
                nb::dict dict = self.to_dict();
                if (include_header)
                {
                    if (self.header.usMessageId == 0) { dict["header"] = nb::none(); }
                    else { dict["header"] = self.header.to_dict(); }
                }
                return dict;
            },
            "include_header"_a = true,
            R"doc(
            Converts the response to a dictionary.
            
            Args:
                include_header: Whether to include the header of the response in the 
                    new representation.

            Returns:
                A dictionary representation of the response.
            )doc")
        // TODO: FINISH REPR
        .def("__repr__", [](const PyResponse& self) { return nb::str("Response()"); })
        .def_prop_ro("header", [](const PyResponse& self) {
            if (self.header.usMessageId == 0) { return nb::none(); }
            return nb::cast(self.header);
        });
}

#pragma endregion
