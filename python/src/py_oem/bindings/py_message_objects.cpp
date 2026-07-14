#include "py_oem/py_message_objects.hpp"

#include <limits>
#include <type_traits>
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

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;
using namespace novatel::edie::py_common;

namespace {
//! Returns true if the raw value corresponds to a defined TIME_STATUS enumerator.
bool IsValidTimeStatus(uint32_t value)
{
    if (value > std::numeric_limits<std::underlying_type_t<TIME_STATUS>>::max()) { return false; }
    switch (static_cast<TIME_STATUS>(value))
    {
    case TIME_STATUS::UNKNOWN:
    case TIME_STATUS::APPROXIMATE:
    case TIME_STATUS::COARSEADJUSTING:
    case TIME_STATUS::COARSE:
    case TIME_STATUS::COARSESTEERING:
    case TIME_STATUS::FREEWHEELING:
    case TIME_STATUS::FINEADJUSTING:
    case TIME_STATUS::FINE:
    case TIME_STATUS::FINEBACKUPSTEERING:
    case TIME_STATUS::FINESTEERING:
    case TIME_STATUS::SATTIME:
    case TIME_STATUS::EXTERN:
    case TIME_STATUS::EXACT: return true;
    default: return false;
    }
}
} // namespace

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

py_common::PyMessageData py_oem::PyEncodableField::PyEncode(ENCODE_FORMAT format)
{
    STATUS status;
    MessageDataStruct message_data = MessageDataStruct();
    const py_oem::DatabaseExtras& extras = py_oem::GetDatabaseExtras(*parentDb);

    // Allocate more space for JSON messages.
    // A TRACKSTAT message can use about 47k bytes when encoded as JSON.
    // FIXME: this is still not safe and there is no effective buffer overflow checking implemented in Encoder.
    uint8_t buffer[MESSAGE_SIZE_MAX * 3];
    auto buf_ptr = reinterpret_cast<unsigned char*>(&buffer);
    uint32_t buf_size = MESSAGE_SIZE_MAX * 3;
    if (extras.rxConfigHandler->IsRxConfigTypeMsg(this->header.usMessageId))
    {
        status = extras.rxConfigHandler->Encode(&buf_ptr, buf_size, this->header, owned_fields(), message_data, format);
    }
    else { status = extras.encoder->Encode(&buf_ptr, buf_size, this->header, owned_fields(), message_data, this->header.format, format); }
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

nb::object py_oem::create_unknown_message_instance(nb::bytes data, py_oem::PyHeader& header)
{
    nb::handle message_pytype = nb::type<py_oem::PyUnknownMessage>();
    nb::object message_pyinst = nb::inst_alloc(message_pytype);
    py_oem::PyUnknownMessage* message_cinst = nb::inst_ptr<py_oem::PyUnknownMessage>(message_pyinst);
    new (message_cinst) py_oem::PyUnknownMessage(header, data);
    nb::inst_mark_ready(message_pyinst);
    return message_pyinst;
}

nb::object py_oem::create_message_instance(py_oem::PyHeader& header, MessageBody&& message_fields, MetaDataStruct& metadata,
                                           py_common::PyMessageDatabase::ConstPtr database)
{

    const MessageDefinition* msgDef = database->GetMsgDef(metadata.usMessageId).get();

    if (metadata.bResponse)
    {

        nb::object response_pyinst = nb::inst_alloc(nb::type<PyResponse>());
        PyResponse* response_cinst = nb::inst_ptr<PyResponse>(response_pyinst);
        bool is_complete = (metadata.eFormat != HEADER_FORMAT::ABB_ASCII);
        new (response_cinst) PyResponse(message_fields, database, header, is_complete, msgDef, metadata.uiMessageCrc);
        nb::inst_mark_ready(response_pyinst);

        return response_pyinst;
    }

    uint32_t crc = metadata.uiMessageCrc;

    nb::handle message_pytype = database->GetMessageType(metadata.usMessageId, crc);
    if (message_pytype.is_none())
    {
        // Fallback to latest CRC
        crc = msgDef->latestMessageCrc;
        message_pytype = database->GetMessageType(msgDef, msgDef->latestMessageCrc);
    }
    nb::object message_pyinst = nb::inst_alloc(message_pytype);
    PyMessage* message_cinst = nb::inst_ptr<PyMessage>(message_pyinst);
    new (message_cinst) PyMessage(std::move(message_fields), database, header, msgDef, crc);

    nb::inst_mark_ready(message_pyinst);
    return message_pyinst;
}

#pragma endregion

#pragma region Bindings

void py_oem::init_header_objects(nb::module_& m)
{
    nb::enum_<py_oem::RECEIVER_STATUS_VERSION>(m, "RECEIVER_STATUS_VERSION", nb::is_arithmetic(),
                                               "The 2-bit version field embedded in the receiver-status word.\n\n"
                                               "https://docs.novatel.com/OEM7/Content/Logs/RXSTATUS.htm#Version_Bits")
        .value("USE_OEM6_ERROR_BITS", py_oem::RECEIVER_STATUS_VERSION::USE_OEM6_ERROR_BITS)
        .value("USE_OEM7_ERROR_BITS", py_oem::RECEIVER_STATUS_VERSION::USE_OEM7_ERROR_BITS)
        .value("RESERVED_1", py_oem::RECEIVER_STATUS_VERSION::RESERVED_1)
        .value("RESERVED_2", py_oem::RECEIVER_STATUS_VERSION::RESERVED_2);

    nb::class_<py_oem::PyRecieverStatus>(m, "RecieverStatus",
                                         "Boolean values indicating information about the state of the reciever.\n\n"
                                         "https://docs.novatel.com/OEM7/Content/Logs/RXSTATUS.htm#Table_ReceiverStatus")
        .def(
            "__init__", [](py_oem::PyRecieverStatus* self, uint32_t value) { new (self) py_oem::PyRecieverStatus(value); }, "value"_a)
        .def(
            "__init__",
            [](py_oem::PyRecieverStatus* self, bool reciever_error, bool temperature_warning, bool voltage_warning, bool antenna_powered,
               bool lna_failure, bool antenna_open_circuit, bool antenna_short_circuit, bool cpu_overload, bool com_buffer_overrun,
               bool spoofing_detected, bool reserved, bool link_overrun, bool input_overrun, bool aux_transmit_overrun,
               bool antenna_gain_out_of_range, bool jammer_detected, bool ins_reset, bool imu_communication_failure, bool gps_almanac_invalid,
               bool position_solution_invalid, bool position_fixed, bool clock_steering_disabled, bool clock_model_invalid,
               bool external_oscillator_locked, bool software_resource_warning, bool tracking_mode_hdr, bool digital_filtering_enabled,
               bool auxiliary_3_event, bool auxiliary_2_event, bool auxiliary_1_event, py_oem::RECEIVER_STATUS_VERSION version_bits) {
                new (self) py_oem::PyRecieverStatus{};
                self->SetRecieverError(reciever_error);
                self->SetTemperatureWarning(temperature_warning);
                self->SetVoltageWarning(voltage_warning);
                self->SetAntennaPowered(antenna_powered);
                self->SetLnaFailure(lna_failure);
                self->SetAntennaOpenCircuit(antenna_open_circuit);
                self->SetAntennaShortCircuit(antenna_short_circuit);
                self->SetCpuOverload(cpu_overload);
                self->SetComBufferOverrun(com_buffer_overrun);
                self->SetSpoofingDetected(spoofing_detected);
                self->SetReserved(reserved);
                self->SetLinkOverrun(link_overrun);
                self->SetInputOverrun(input_overrun);
                self->SetAuxTransmitOverrun(aux_transmit_overrun);
                self->SetAntennaGainOutOfRange(antenna_gain_out_of_range);
                self->SetJammerDetected(jammer_detected);
                self->SetInsReset(ins_reset);
                self->SetImuCommunicationFailure(imu_communication_failure);
                self->SetGpsAlmanacInvalid(gps_almanac_invalid);
                self->SetPositionSolutionInvalid(position_solution_invalid);
                self->SetPositionFixed(position_fixed);
                self->SetClockSteeringDisabled(clock_steering_disabled);
                self->SetClockModelInvalid(clock_model_invalid);
                self->SetExternalOscillatorLocked(external_oscillator_locked);
                self->SetSoftwareResourceWarning(software_resource_warning);
                self->SetTrackingModeHdr(tracking_mode_hdr);
                self->SetDigitalFilteringEnabled(digital_filtering_enabled);
                self->SetAuxiliary3Event(auxiliary_3_event);
                self->SetAuxiliary2Event(auxiliary_2_event);
                self->SetAuxiliary1Event(auxiliary_1_event);
                self->SetVersionBits(version_bits);
            },
            nb::kw_only(), "reciever_error"_a = false, "temperature_warning"_a = false, "voltage_warning"_a = false, "antenna_powered"_a = false,
            "lna_failure"_a = false, "antenna_open_circuit"_a = false, "antenna_short_circuit"_a = false, "cpu_overload"_a = false,
            "com_buffer_overrun"_a = false, "spoofing_detected"_a = false, "reserved"_a = false, "link_overrun"_a = false, "input_overrun"_a = false,
            "aux_transmit_overrun"_a = false, "antenna_gain_out_of_range"_a = false, "jammer_detected"_a = false, "ins_reset"_a = false,
            "imu_communication_failure"_a = false, "gps_almanac_invalid"_a = false, "position_solution_invalid"_a = false, "position_fixed"_a = false,
            "clock_steering_disabled"_a = false, "clock_model_invalid"_a = false, "external_oscillator_locked"_a = false,
            "software_resource_warning"_a = false, "tracking_mode_hdr"_a = false, "digital_filtering_enabled"_a = false,
            "auxiliary_3_event"_a = false, "auxiliary_2_event"_a = false, "auxiliary_1_event"_a = false,
            "version_bits"_a = py_oem::RECEIVER_STATUS_VERSION::USE_OEM6_ERROR_BITS)
        .def(
            "__eq__",
            [](const py_oem::PyRecieverStatus& self, nb::object other) {
                if (!nb::isinstance<py_oem::PyRecieverStatus>(other)) { return false; }
                return self.value == nb::cast<const py_oem::PyRecieverStatus&>(other).value;
            },
            "other"_a.none(true))
        .def_rw("raw_value", &py_oem::PyRecieverStatus::value, "The raw 32-bit receiver status word.")
        .def_prop_rw("reciever_error", &py_oem::PyRecieverStatus::GetRecieverError, &py_oem::PyRecieverStatus::SetRecieverError,
                     "Bit 0. True if a receiver error is present (see the RXSTATUS error word), False if no error.")
        .def_prop_rw("temperature_warning", &py_oem::PyRecieverStatus::GetTemperatureWarning, &py_oem::PyRecieverStatus::SetTemperatureWarning,
                     "Bit 1. True if a temperature warning is active, False if within specifications.")
        .def_prop_rw("voltage_warning", &py_oem::PyRecieverStatus::GetVoltageWarning, &py_oem::PyRecieverStatus::SetVoltageWarning,
                     "Bit 2. True if a voltage supply warning is active, False if OK.")
        .def_prop_rw("antenna_powered", &py_oem::PyRecieverStatus::GetAntennaPowered, &py_oem::PyRecieverStatus::SetAntennaPowered,
                     "Bit 3, primary antenna power status (see the ANTENNAPOWER command). NOTE: this exposes the raw status bit, which is SET "
                     "when the antenna is not powered. True therefore means the antenna is NOT powered, False means it is powered.")
        .def_prop_rw("lna_failure", &py_oem::PyRecieverStatus::GetLnaFailure, &py_oem::PyRecieverStatus::SetLnaFailure,
                     "Bit 4. True if an LNA failure is detected, False if OK.")
        .def_prop_rw("antenna_open_circuit", &py_oem::PyRecieverStatus::GetAntennaOpenCircuit, &py_oem::PyRecieverStatus::SetAntennaOpenCircuit,
                     "Bit 5. True if the primary antenna is open (disconnected), False if OK.")
        .def_prop_rw("antenna_short_circuit", &py_oem::PyRecieverStatus::GetAntennaShortCircuit, &py_oem::PyRecieverStatus::SetAntennaShortCircuit,
                     "Bit 6. True if a primary antenna short circuit is detected, False if OK.")
        .def_prop_rw("cpu_overload", &py_oem::PyRecieverStatus::GetCpuOverload, &py_oem::PyRecieverStatus::SetCpuOverload,
                     "Bit 7. True if the CPU is overloaded, False if no overload.")
        .def_prop_rw(
            "com_buffer_overrun", &py_oem::PyRecieverStatus::GetComBufferOverrun, &py_oem::PyRecieverStatus::SetComBufferOverrun,
            "Bit 8. True if a COM port transmit buffer overrun occurred, False if OK. See the AUX2 status bits for individual COM port status.")
        .def_prop_rw("spoofing_detected", &py_oem::PyRecieverStatus::GetSpoofingDetected, &py_oem::PyRecieverStatus::SetSpoofingDetected,
                     "Bit 9. True if spoofing is detected, False if not detected.")
        .def_prop_rw("reserved", &py_oem::PyRecieverStatus::GetReserved, &py_oem::PyRecieverStatus::SetReserved, "Bit 10. Reserved.")
        .def_prop_rw("link_overrun", &py_oem::PyRecieverStatus::GetLinkOverrun, &py_oem::PyRecieverStatus::SetLinkOverrun,
                     "Bit 11. True if any of the USB, ICOM, CCOM, NCOM or File ports are overrun, False if no overrun. See the AUX1, AUX2 and AUX3 "
                     "status bits for the specific overrun port.")
        .def_prop_rw("input_overrun", &py_oem::PyRecieverStatus::GetInputOverrun, &py_oem::PyRecieverStatus::SetInputOverrun,
                     "Bit 12. True if a receiver port (COM, USB, ICOM or NCOM) experienced an input overrun, False if no overrun.")
        .def_prop_rw("aux_transmit_overrun", &py_oem::PyRecieverStatus::GetAuxTransmitOverrun, &py_oem::PyRecieverStatus::SetAuxTransmitOverrun,
                     "Bit 13. True if an auxiliary transmit overrun occurred, False if no overrun.")
        .def_prop_rw("antenna_gain_out_of_range", &py_oem::PyRecieverStatus::GetAntennaGainOutOfRange,
                     &py_oem::PyRecieverStatus::SetAntennaGainOutOfRange,
                     "Bit 14. True if the antenna gain state is out of range, False if OK. See the AUX3 status bits for the antenna gain status.")
        .def_prop_rw("jammer_detected", &py_oem::PyRecieverStatus::GetJammerDetected, &py_oem::PyRecieverStatus::SetJammerDetected,
                     "Bit 15. True if a jammer is detected, False if OK. See the AUX1 status bits for individual RF status.")
        .def_prop_rw("ins_reset", &py_oem::PyRecieverStatus::GetInsReset, &py_oem::PyRecieverStatus::SetInsReset,
                     "Bit 16. True if an INS reset occurred, False if no INS reset.")
        .def_prop_rw("imu_communication_failure", &py_oem::PyRecieverStatus::GetImuCommunicationFailure,
                     &py_oem::PyRecieverStatus::SetImuCommunicationFailure, "Bit 17. True if there is no IMU communication, False if no error.")
        .def_prop_rw("gps_almanac_invalid", &py_oem::PyRecieverStatus::GetGpsAlmanacInvalid, &py_oem::PyRecieverStatus::SetGpsAlmanacInvalid,
                     "Bit 18. True if the GPS almanac / UTC is invalid, False if valid.")
        .def_prop_rw("position_solution_invalid", &py_oem::PyRecieverStatus::GetPositionSolutionInvalid,
                     &py_oem::PyRecieverStatus::SetPositionSolutionInvalid, "Bit 19. True if the position solution is invalid, False if valid.")
        .def_prop_rw("position_fixed", &py_oem::PyRecieverStatus::GetPositionFixed, &py_oem::PyRecieverStatus::SetPositionFixed,
                     "Bit 20. True if the position is fixed (see the FIX command), False if not fixed.")
        .def_prop_rw("clock_steering_disabled", &py_oem::PyRecieverStatus::GetClockSteeringDisabled,
                     &py_oem::PyRecieverStatus::SetClockSteeringDisabled, "Bit 21. True if clock steering is disabled, False if enabled.")
        .def_prop_rw("clock_model_invalid", &py_oem::PyRecieverStatus::GetClockModelInvalid, &py_oem::PyRecieverStatus::SetClockModelInvalid,
                     "Bit 22. True if the clock model is invalid, False if valid.")
        .def_prop_rw("external_oscillator_locked", &py_oem::PyRecieverStatus::GetExternalOscillatorLocked,
                     &py_oem::PyRecieverStatus::SetExternalOscillatorLocked, "Bit 23. True if the external oscillator is locked, False if unlocked.")
        .def_prop_rw("software_resource_warning", &py_oem::PyRecieverStatus::GetSoftwareResourceWarning,
                     &py_oem::PyRecieverStatus::SetSoftwareResourceWarning, "Bit 24. True if a software resource warning is active, False if OK.")
        .def_prop_rw(
            "version_bits", &py_oem::PyRecieverStatus::GetVersionBits, &py_oem::PyRecieverStatus::SetVersionBits,
            "Bits 25-26. The 2-bit version field that determines how the status/error bits are interpreted (OEM6-or-earlier vs OEM7 format).")
        .def_prop_rw("tracking_mode_hdr", &py_oem::PyRecieverStatus::GetTrackingModeHdr, &py_oem::PyRecieverStatus::SetTrackingModeHdr,
                     "Bit 27. True if the receiver is in HDR (High Dynamic Range) tracking mode, False if in normal tracking mode.")
        .def_prop_rw("digital_filtering_enabled", &py_oem::PyRecieverStatus::GetDigitalFilteringEnabled,
                     &py_oem::PyRecieverStatus::SetDigitalFilteringEnabled, "Bit 28. True if digital filtering is enabled, False if disabled.")
        .def_prop_rw("auxiliary_3_event", &py_oem::PyRecieverStatus::GetAuxiliary3Event, &py_oem::PyRecieverStatus::SetAuxiliary3Event,
                     "Bit 29. True if an Auxiliary 3 status event occurred, False if no event. See the AUX3 status word.")
        .def_prop_rw("auxiliary_2_event", &py_oem::PyRecieverStatus::GetAuxiliary2Event, &py_oem::PyRecieverStatus::SetAuxiliary2Event,
                     "Bit 30. True if an Auxiliary 2 status event occurred, False if no event. See the AUX2 status word.")
        .def_prop_rw("auxiliary_1_event", &py_oem::PyRecieverStatus::GetAuxiliary1Event, &py_oem::PyRecieverStatus::SetAuxiliary1Event,
                     "Bit 31. True if an Auxiliary 1 status event occurred, False if no event. See the AUX1 status word.")
        .def("__repr__", [](const py_oem::PyRecieverStatus& self) {
            return nb::str("RecieverStatus(") + nb::str(nb::module_::import_("builtins").attr("hex")(self.value)) + nb::str(")");
        });

    nb::class_<py_oem::PyMessageTypeField>(m, "MessageType",
                                           "A message field which provides information about its source, format, and whether it is a response.")
        .def(nb::init_implicit<uint8_t>())
        .def(
            "__init__",
            [](py_oem::PyMessageTypeField* self, bool is_response, MESSAGE_FORMAT format, uint8_t sibling_id) {
                uint8_t value = 0;
                if (is_response) { value |= static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE); }
                value |= (static_cast<uint8_t>(format) << 5) & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MSGFORMAT);
                value |= sibling_id & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MEASSRC);
                new (self) py_oem::PyMessageTypeField(value);
            },
            nb::kw_only(), "is_response"_a = false, "format"_a = MESSAGE_FORMAT::BINARY, "sibling_id"_a = uint8_t{0})
        .def_ro("raw_value", &py_oem::PyMessageTypeField::value, "The raw message-type byte.")
        .def(
            "__eq__",
            [](const py_oem::PyMessageTypeField& self, nb::object other) {
                if (!nb::isinstance<py_oem::PyMessageTypeField>(other)) { return false; }
                return self.value == nb::cast<const py_oem::PyMessageTypeField&>(other).value;
            },
            "other"_a.none(true))
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
        .def(
            "__init__",
            [](py_oem::PyHeader* self, py_oem::PyMessageTypeField message_type, uint32_t port_address, uint16_t sequence, uint8_t idle_time,
               uint32_t time_status, uint16_t week, double milliseconds, std::variant<py_oem::PyRecieverStatus, uint32_t> receiver_status,
               uint16_t receiver_sw_version) {
                new (self) py_oem::PyHeader{};
                self->ucMessageType = message_type.value;
                self->uiPortAddress = port_address;
                self->usSequence = sequence;
                self->ucIdleTime = idle_time;
                self->uiTimeStatus = time_status;
                self->usWeek = week;
                self->dMilliseconds = milliseconds;
                self->uiReceiverStatus = std::holds_alternative<py_oem::PyRecieverStatus>(receiver_status)
                                             ? std::get<py_oem::PyRecieverStatus>(receiver_status).value
                                             : std::get<uint32_t>(receiver_status);
                self->usReceiverSwVersion = receiver_sw_version;
            },
            nb::sig("def __init__(self, *, message_type: MessageType | int = 0, port_address: int = 0, "
                    "sequence: int = 0, idle_time: int = 0, "
                    "time_status: common_bindings.TIME_STATUS | int = common_bindings.TIME_STATUS.UNKNOWN, "
                    "week: int = 0, milliseconds: float = 0.0, receiver_status: RecieverStatus | int = 0, "
                    "receiver_sw_version: int = 0) -> None"),
            nb::kw_only(), "message_type"_a = py_oem::PyMessageTypeField(0), "port_address"_a = uint32_t{0},
            "sequence"_a = uint16_t{0}, "idle_time"_a = uint8_t{0}, "time_status"_a = static_cast<uint32_t>(TIME_STATUS::UNKNOWN),
            "week"_a = uint16_t{0}, "milliseconds"_a = double{0.0}, "receiver_status"_a = uint32_t{0},
            "receiver_sw_version"_a = uint16_t{0})
        .def_ro("message_id", &py_oem::PyHeader::usMessageId, "The Message ID number.")
        .def_prop_rw(
            "message_type", &py_oem::PyHeader::GetPyMessageType,
            [](py_oem::PyHeader& self, py_oem::PyMessageTypeField value) { self.ucMessageType = value.value; },
            "Information regarding the type of the message.")
        .def_rw("port_address", &py_oem::PyHeader::uiPortAddress, "The port the message was sent from.")
        .def_ro("length", &py_oem::PyHeader::usLength, "The length of the message. Will be 0 if unknown.")
        .def_rw("sequence", &py_oem::PyHeader::usSequence, "Number of remaning related messages following this one. Will be 0 for most messages.")
        .def_rw("idle_time", &py_oem::PyHeader::ucIdleTime, "Time that the processor is idle. Divide by two to get the percentage.")
        .def_prop_rw(
            "time_status",
            [](const py_oem::PyHeader& self) -> nb::object {
                if (IsValidTimeStatus(self.uiTimeStatus)) { return nb::cast(static_cast<TIME_STATUS>(self.uiTimeStatus)); }
                return nb::cast(self.uiTimeStatus);
            },
            [](py_oem::PyHeader& self, uint32_t value) { self.uiTimeStatus = value; }, nb::sig("def time_status(self) -> TIME_STATUS | int"),
            "The quality of the GPS reference time.")
        .def_rw("week", &py_oem::PyHeader::usWeek, "GPS reference week number.")
        .def_rw("milliseconds", &py_oem::PyHeader::dMilliseconds, "Milliseconds from the beginning of the GPS reference week.")
        .def_prop_rw(
            "receiver_status", &py_oem::PyHeader::GetRecieverStatus,
            [](py_oem::PyHeader& self, std::variant<py_oem::PyRecieverStatus, uint32_t> value) {
                self.uiReceiverStatus = std::holds_alternative<py_oem::PyRecieverStatus>(value) ? std::get<py_oem::PyRecieverStatus>(value).value
                                                                                                : std::get<uint32_t>(value);
            },
            "32-bits representing the status of various hardware and software components of the receiver.")
        .def_ro("message_definition_crc", &py_oem::PyHeader::uiMessageDefinitionCrc,
                "A value for validating the message definition used for decoding.")
        .def_rw("receiver_sw_version", &py_oem::PyHeader::usReceiverSwVersion, "A value (0 - 65535) representing the receiver software build number.")
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

    auto message_class = nb::class_<py_oem::PyMessage, py_common::PyField>(m, "Message");
    message_class
        .def_static(
            "__new__",
            [](nb::handle cls, py_oem::PyHeader* header, nb::kwargs kwargs) {
                PyMessageDatabase::Ptr database = nb::cast<PyMessageDatabase::Ptr>(cls.attr("_owner_db"));

                if (!database) { throw py_common::FailureException("Constructor could not resolve owning MessageDatabase for this type."); }

                const py_common::MessageTypeLookupEntry* type_lookup = database->GetMessageTypeLookup(cls);
                if (!type_lookup || !type_lookup->def)
                {
                    throw py_common::FailureException("Constructor could not resolve MessageDefinition for this type.");
                }

                nb::object message_pyinst = nb::inst_alloc(cls);
                py_oem::PyMessage* message_cinst = nb::inst_ptr<py_oem::PyMessage>(message_pyinst);
                new (message_cinst) py_oem::PyMessage(database, type_lookup->def, type_lookup->crc);
                nb::inst_mark_ready(message_pyinst);

                // All construction work happens here rather than in a separate
                // __init__ (which is repointed to object.__init__, a fast C no-op).
                if (header != nullptr) { message_cinst->header = *header; }
                // Override the header's identification fields from the message's own
                // definition so the encoded message always identifies as itself.
                message_cinst->header.usMessageId = message_cinst->messageDef->logID;
                message_cinst->header.uiMessageDefinitionCrc = message_cinst->messageCrc;

                // Set the values for all subfields
                for (auto kv : kwargs) { message_cinst->setattr(nb::cast<nb::str>(kv.first), kv.second); }
                return message_pyinst;
            },
            "cls"_a, nb::arg("header") = nb::none(), "kwargs"_a)
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
        .def_prop_ro("name", &py_oem::PyEncodableField::name, "The type of message it is.");

    // No Python-level __init__: the __new__ above does all construction work.
    // Repoint __init__ to object.__init__ (a fast C no-op) so nanobind's default
    // tp_init does not raise and no extra trampoline runs per construction.
    // Generated message subclasses inherit this slot. (Base PyField already does
    // the same, but PyMessage overrides __new__, so set it here too.)
    message_class.attr("__init__") = nb::handle(reinterpret_cast<PyObject*>(&PyBaseObject_Type)).attr("__init__");

    auto& familyRegistrations = py_common::GetMessageFamilyRegistrations();
    familyRegistrations["OEM"] = py_common::MessageFamilyRegistration{nb::type<py_oem::PyMessage>(), &py_oem::AllocateDatabaseExtras};
    familyRegistrations[""] = py_common::MessageFamilyRegistration{nb::type<py_oem::PyMessage>(), &py_oem::AllocateDatabaseExtras};

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
        .def_prop_ro("name", &py_oem::PyEncodableField::name, "The type of message it is.")
        .def_prop_ro("response_id", &py_oem::PyResponse::GetResponseId)
        .def_prop_ro("response_string", &py_oem::PyResponse::GetResponseString)
        // typehints in stubgen_pattern.txt
        .def_prop_ro("response_enum", &py_oem::PyResponse::GetEnumValue);
}

#pragma endregion
