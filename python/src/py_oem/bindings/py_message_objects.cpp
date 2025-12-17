#include "py_oem/py_message_objects.hpp"

#include <variant>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_common/field_objects.hpp"
#include "py_common/py_message_data.hpp"
#include "py_common/unknown_bytes.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/message_database.hpp"
#include "py_oem/message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;
using namespace novatel::edie::py_common;

NB_MAKE_OPAQUE(std::vector<FieldContainer>);

#pragma region PyHeader Methods

nb::dict py_oem::PyHeader::to_dict() const
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

#pragma region PyMessageMethods

// Static initalization of this struct will populate the BasePythonTypes map with a way to access the oem PyMessage type
struct OEMMessageStyleTypeInitializer
{
    OEMMessageStyleTypeInitializer()
    {
        auto& baseTypes = py_common::GetBasePythonTypes();
        std::function<nb::handle()> typeGetter = []() { return nb::type<py_oem::PyMessage>(); };
        baseTypes["OEM4_MESSAGE_STYLE"] = typeGetter;
        baseTypes["OEM4_COMPRESSED_STYLE"] = typeGetter;
    }
};
static OEMMessageStyleTypeInitializer oem_message_style_type_initializer;

py_common::PyMessageData py_oem::PyEncodableField::PyEncode(ENCODE_FORMAT format)
{
    STATUS status;
    MessageDataStruct message_data = MessageDataStruct();
    RxConfigHandler* pclRxConfigHandler = this->encoderDb->GetRxConfigHandler();

    // Allocate more space for JSON messages.
    // A TRACKSTAT message can use about 47k bytes when encoded as JSON.
    // FIXME: this is still not safe and there is no effective buffer overflow checking implemented in Encoder.
    uint8_t buffer[MESSAGE_SIZE_MAX * 3];
    auto buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
    uint32_t buf_size = MESSAGE_SIZE_MAX * 3;
    if (pclRxConfigHandler->IsRxConfigTypeMsg(this->header.usMessageId))
    {
        status = this->encoderDb->GetRxConfigHandler()->Encode(&buf_ptr, buf_size, this->header, this->fields, message_data, format);
    }
    else
    {
        status = this->encoderDb->GetEncoder()->Encode(&buf_ptr, buf_size, this->header, this->fields, message_data, this->header.format, format);
    }
    throw_exception_from_status(status);
    return py_common::PyMessageData(message_data);
}

py_common::PyMessageData py_oem::PyEncodableField::encode(ENCODE_FORMAT fmt) { return PyEncode(fmt); }

py_common::PyMessageData py_oem::PyEncodableField::to_ascii() { return PyEncode(ENCODE_FORMAT::ASCII); }

py_common::PyMessageData py_oem::PyEncodableField::to_abbrev_ascii() { return PyEncode(ENCODE_FORMAT::ABBREV_ASCII); }

py_common::PyMessageData py_oem::PyEncodableField::to_binary() { return PyEncode(ENCODE_FORMAT::BINARY); }

py_common::PyMessageData py_oem::PyEncodableField::to_flattened_binary() { return PyEncode(ENCODE_FORMAT::FLATTENED_BINARY); }

py_common::PyMessageData py_oem::PyEncodableField::to_json() { return PyEncode(ENCODE_FORMAT::JSON); }

#pragma endregion

#pragma region Message Constructor Functions

nb::object py_oem::create_unknown_message_instance(nb::bytes data, py_oem::PyHeader& header, py_oem::PyMessageDatabase::ConstPtr database)
{
    nb::handle message_pytype = nb::type<py_oem::PyUnknownMessage>();
    nb::object message_pyinst = nb::inst_alloc(message_pytype);
    py_oem::PyUnknownMessage* message_cinst = nb::inst_ptr<py_oem::PyUnknownMessage>(message_pyinst);
    new (message_cinst) py_oem::PyUnknownMessage(database, header, data);
    nb::inst_mark_ready(message_pyinst);
    return message_pyinst;
}

nb::object py_oem::create_message_instance(py_oem::PyHeader& header, std::vector<FieldContainer>&& message_fields, MetaDataStruct& metadata,
                                           py_oem::PyMessageDatabase::ConstPtr database)
{

    nb::handle message_pytype;

    const std::string message_name = metadata.messageName;

    bool has_ptype = true;

    if (metadata.bResponse)
    {

        nb::object response_pyinst = nb::inst_alloc(nb::type<PyResponse>());
        PyResponse* response_cinst = nb::inst_ptr<PyResponse>(response_pyinst);
        bool is_complete = (metadata.eFormat != HEADER_FORMAT::ABB_ASCII);
        new (response_cinst) PyResponse(message_name, message_fields, database, header, is_complete);
        nb::inst_mark_ready(response_pyinst);

        return response_pyinst;
    }
    try
    {
        py_common::PyMessageType* message_type_struct = database->GetMessagesByNameDict().at(message_name);
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
    catch (const std::out_of_range&)
    {
        // This case should never happen, if it does there is a bug
        throw std::runtime_error("Message name '" + message_name + "' not found in the JSON database");
    }
    nb::object message_pyinst = nb::inst_alloc(message_pytype);
    PyMessage* message_cinst = nb::inst_ptr<PyMessage>(message_pyinst);
    new (message_cinst) PyMessage(message_name, has_ptype, std::move(message_fields), database, header);

    nb::inst_mark_ready(message_pyinst);
    return message_pyinst;
}

#pragma endregion

#pragma region Bindings

void py_oem::init_header_objects(nb::module_& m)
{
    nb::class_<py_oem::PyRecieverStatus>(m, "RecieverStatus", "Boolean values indicating information about the state of the reciever.")
        .def_ro("raw_value", &py_oem::PyRecieverStatus::value)
        .def_prop_ro("reciever_error", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000001) != 0; })
        .def_prop_ro("temperature_warning", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000002) != 0; })
        .def_prop_ro("voltage_warning", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000004) != 0; })
        .def_prop_ro("antenna_powered", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000008) != 0; })
        .def_prop_ro("lna_failure", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000010) != 0; })
        .def_prop_ro("antenna_open_circuit", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000020) != 0; })
        .def_prop_ro("antenna_short_circuit", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000040) != 0; })
        .def_prop_ro("cpu_overload", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000080) != 0; })
        .def_prop_ro("com_buffer_overrun", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000100) != 0; })
        .def_prop_ro("spoofing_detected", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000200) != 0; })
        // Reserved flag does not need a function
        .def_prop_ro("link_overrun", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00000800) != 0; })
        .def_prop_ro("input_overrun", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00001000) != 0; })
        .def_prop_ro("aux_transmit_overrun", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00002000) != 0; })
        .def_prop_ro("antenna_gain_out_of_range", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00004000) != 0; })
        .def_prop_ro("jammer_detected", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00008000) != 0; })
        .def_prop_ro("ins_reset", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00010000) != 0; })
        .def_prop_ro("imu_communication_failure", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00020000) != 0; })
        .def_prop_ro("gps_almanac_invalid", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00040000) != 0; })
        .def_prop_ro("position_solution_invalid", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00080000) != 0; })
        .def_prop_ro("position_fixed", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00100000) != 0; })
        .def_prop_ro("clock_steering_disabled", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00200000) != 0; })
        .def_prop_ro("clock_model_invalid", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00400000) != 0; })
        .def_prop_ro("external_oscillator_locked", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x00800000) != 0; })
        .def_prop_ro("software_resource_warning", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x01000000) != 0; })
        .def_prop_ro("tracking_mode_hdr", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x08000000) != 0; })
        .def_prop_ro("digital_filtering_enabled", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x10000000) != 0; })
        .def_prop_ro("auxiliary_3_event", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x20000000) != 0; })
        .def_prop_ro("auxiliary_2_event", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x40000000) != 0; })
        .def_prop_ro("auxiliary_1_event", [](const py_oem::PyRecieverStatus& self) { return (self.value & 0x80000000) != 0; })
        .def_prop_ro("version_bits", [](const py_oem::PyRecieverStatus& self) { return (self.value >> 25) & 0x06000000; })
        .def("__repr__", [](const py_oem::PyRecieverStatus& self) {
            return nb::str("RecieverStatus(") + nb::str(nb::module_::import_("builtins").attr("hex")(self.value)) + nb::str(")");
        });

    nb::class_<py_oem::PyMessageTypeField>(m, "MessageType",
                                           "A message field which provides information about its source, format, and whether it is a response.")
        .def("__repr__",
             [](nb::handle_t<py_oem::PyMessageTypeField> self) {
                 std::string sibling_id = nb::cast<nb::str>(self.attr("sibling_id").attr("__repr__")()).c_str();
                 std::string format = nb::cast<nb::str>(self.attr("format").attr("__repr__")()).c_str();
                 std::string response = nb::cast<nb::str>(self.attr("is_response").attr("__repr__")()).c_str();

                 return "MessageType(sibling_id=" + sibling_id + ", format=" + format + ", response=" + response + ")";
             })
        .def_prop_ro("is_response", &py_oem::PyMessageTypeField::IsResponse, "Whether the message is a response.")
        .def_prop_ro("format", &py_oem::PyMessageTypeField::GetFormat, "The original format of the message.")
        .def_prop_ro("sibling_id", &py_oem::PyMessageTypeField::GetSiblingId, "Where the message originates from.")
        .def_prop_ro("source", &py_oem::PyMessageTypeField::GetMeasurementSource, "Where the message originates from.");

    nb::class_<py_oem::PyHeader>(m, "Header")
        .def_ro("message_id", &py_oem::PyHeader::usMessageId, "The Message ID number.")
        .def_prop_ro("message_type", &py_oem::PyHeader::GetPyMessageType, nb::sig("def message_type(self) -> MessageType"),
                     "Information regarding the type of the message.")
        .def_ro("port_address", &py_oem::PyHeader::uiPortAddress, "The port the message was sent from.")
        .def_ro("length", &py_oem::PyHeader::usLength, "The length of the message. Will be 0 if unknown.")
        .def_ro("sequence", &py_oem::PyHeader::usSequence, "Number of remaning related messages following this one. Will be 0 for most messages.")
        .def_ro("idle_time", &py_oem::PyHeader::ucIdleTime, "Time that the processor is idle. Divide by two to get the percentage.")
        .def_prop_ro(
            "time_status", [](const py_oem::PyHeader& self) { return TIME_STATUS(self.uiTimeStatus); }, "The quality of the GPS reference time.")
        .def_ro("week", &py_oem::PyHeader::usWeek, "GPS reference wekk number.")
        .def_ro("milliseconds", &py_oem::PyHeader::dMilliseconds, "Milliseconds from the beginning of the GPS reference week.")
        .def_prop_ro("receiver_status", &py_oem::PyHeader::GetRecieverStatus,
                     "32-bits representing the status of various hardware and software components of the receiver.")
        .def_ro("message_definition_crc", &py_oem::PyHeader::uiMessageDefinitionCrc,
                "A value for validating the message definition used for decoding.")
        .def_ro("receiver_sw_version", &py_oem::PyHeader::usReceiverSwVersion, "A value (0 - 65535) representing the receiver software build number.")
        .def(
            "to_dict", [](const py_oem::PyHeader& self) { return self.to_dict(); },
            R"doc(
            Converts the header to a dictionary.

            Returns:
                A dictionary representation of the header.
            )doc")
        .def("__repr__", [](const nb::handle self) {
            std::vector<nb::str> fields = {nb::str("message_id"),
                                           nb::str("message_type"),
                                           nb::str("port_address"),
                                           nb::str("length"),
                                           nb::str("sequence"),
                                           nb::str("idle_time"),
                                           nb::str("time_status"),
                                           nb::str("week"),
                                           nb::str("milliseconds"),
                                           nb::str("receiver_status"),
                                           nb::str("message_definition_crc"),
                                           nb::str("receiver_sw_version")};
            nb::str header_repr = nb::str("Header(");
            for (size_t i = 0; i < fields.size(); ++i)
            {
                header_repr += fields[i] + nb::str("=") + nb::repr(self.attr(fields[i]));
                if (i != fields.size() - 1) { header_repr += nb::str(", "); }
            }
            header_repr += nb::str(")");
            return header_repr;
        });
}

void py_oem::init_message_objects(nb::module_& m)
{

    nb::class_<py_oem::PyGpsTime>(m, "GpsTime", "A GPS reference time.")
        .def(nb::init())
        .def(nb::init<uint16_t, double>(), "week"_a, "milliseconds"_a = TIME_STATUS::UNKNOWN)
        .def(nb::init<uint16_t, double, TIME_STATUS>(), "week"_a, "milliseconds"_a, "time_status"_a = TIME_STATUS::UNKNOWN)
        .def("__repr__",
             [](py_oem::PyGpsTime& self) { return "GPSTime(" + std::to_string(self.week) + ", " + std::to_string(self.milliseconds) + ")"; })
        .def_rw("week", &py_oem::PyGpsTime::week, "GPS reference week number.")
        .def_rw("milliseconds", &py_oem::PyGpsTime::milliseconds, "Milliseconds from the beginning of the GPS reference week.")
        .def_rw("status", &py_oem::PyGpsTime::time_status, "The quality of the GPS reference time.");

    nb::class_<py_oem::PyUnknownMessage>(m, "UnknownMessage")
        .def("__repr__",
             [](const py_oem::PyUnknownMessage self) {
                 std::string byte_rep = nb::str(self.payload.attr("__repr__")()).c_str();
                 return "IncompleteMessage(payload=" + byte_rep + ")";
             })
        .def_ro("header", &py_oem::PyUnknownMessage::header, "The header of the message.")
        .def_ro("payload", &py_oem::PyUnknownMessage::payload, "The undecoded bytes that make up the message's payload.")
        .def(
            "to_dict",
            [](const py_oem::PyUnknownMessage& self) {
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

    nb::class_<py_oem::PyMessage, py_common::PyField>(m, "Message")
        .def("encode", &py_oem::PyMessage::encode)
        .def("to_ascii", &py_oem::PyMessage::to_ascii)
        .def("to_abbrev_ascii", &py_oem::PyMessage::to_abbrev_ascii)
        .def("to_binary", &py_oem::PyMessage::to_binary)
        .def("to_flattened_binary", &py_oem::PyMessage::to_flattened_binary)
        .def("to_json", &py_oem::PyMessage::to_json)
        .def(
            "to_dict",
            [](const py_oem::PyMessage& self, bool include_header) {
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
        .def_ro("header", &py_oem::PyMessage::header, "The header of the message.")
        .def_ro("name", &py_oem::PyMessage::name, "The type of message it is.");

    nb::class_<py_oem::PyResponse>(m, "Response")
        .def("encode", &py_oem::PyResponse::encode)
        .def("to_ascii", &py_oem::PyResponse::to_ascii)
        .def("to_abbrev_ascii", &py_oem::PyResponse::to_abbrev_ascii)
        .def("to_binary", &py_oem::PyResponse::to_binary)
        .def("to_flattended_binary", &py_oem::PyResponse::to_flattened_binary)
        .def("to_json", &py_oem::PyResponse::to_json)
        .def(
            "to_dict",
            [](const py_oem::PyResponse& self, bool include_header) {
                nb::dict dict = self.to_dict();
                if (include_header)
                {
                    if (!self.complete) { dict["header"] = nb::none(); }
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
        .def("__repr__",
             [](py_oem::PyResponse& self) {
                 nb::str repr = nb::str("Response(");
                 repr += nb::str(self.GetResponseString().c_str());
                 repr += nb::str(")");
                 return repr;
             })
        .def_prop_ro("header",
                     [](const py_oem::PyResponse& self) {
                         if (!self.complete) { return nb::none(); }
                         return nb::cast(self.header);
                     })
        .def_ro("name", &py_oem::PyResponse::name, "The type of message it is.")
        .def_prop_ro("response_id", &py_oem::PyResponse::GetResponseId)
        .def_prop_ro("response_string", &py_oem::PyResponse::GetResponseString)
        // typehints in stubgen_pattern.txt
        .def_prop_ro("response_enum", &py_oem::PyResponse::GetEnumValue);
}

#pragma endregion
