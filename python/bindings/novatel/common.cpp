#include "novatel_edie/decoders/oem/common.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_common(nb::module_& m)
{
    nb::enum_<novatel::edie::HEADER_FORMAT>(m, "HEADER_FORMAT")
        .value("UNKNOWN", novatel::edie::HEADER_FORMAT::UNKNOWN)
        .value("BINARY", novatel::edie::HEADER_FORMAT::BINARY)
        .value("SHORT_BINARY", novatel::edie::HEADER_FORMAT::SHORT_BINARY)
        .value("PROPRIETARY_BINARY", novatel::edie::HEADER_FORMAT::PROPRIETARY_BINARY)
        .value("ASCII", novatel::edie::HEADER_FORMAT::ASCII)
        .value("SHORT_ASCII", novatel::edie::HEADER_FORMAT::SHORT_ASCII)
        .value("ABB_ASCII", novatel::edie::HEADER_FORMAT::ABB_ASCII)
        .value("NMEA", novatel::edie::HEADER_FORMAT::NMEA)
        .value("JSON", novatel::edie::HEADER_FORMAT::JSON)
        .value("SHORT_ABB_ASCII", novatel::edie::HEADER_FORMAT::SHORT_ABB_ASCII)
        .value("ALL", novatel::edie::HEADER_FORMAT::ALL);

    nb::class_<novatel::edie::MessageDataStruct>(m, "MessageDataStruct")
        .def(nb::init<>())
        .def_prop_ro("message_header",
                     [](novatel::edie::MessageDataStruct& self) { return nb::bytes((char*)self.pucMessageHeader, self.uiMessageHeaderLength); })
        .def_prop_ro("message_body",
                     [](novatel::edie::MessageDataStruct& self) { return nb::bytes((char*)self.pucMessageBody, self.uiMessageBodyLength); })
        .def_prop_ro("message", [](novatel::edie::MessageDataStruct& self) { return nb::bytes((char*)self.pucMessage, self.uiMessageLength); });

    nb::class_<oem::MetaDataStruct>(m, "MetaDataStruct")
        .def(nb::init<>())
        .def(nb::init<novatel::edie::HEADER_FORMAT, uint32_t>(), "format"_a, "length"_a)
        .def_rw("format", &oem::MetaDataStruct::eFormat)
        .def_rw("measurement_source", &oem::MetaDataStruct::eMeasurementSource)
        .def_rw("time_status", &oem::MetaDataStruct::eTimeStatus)
        .def_rw("response", &oem::MetaDataStruct::bResponse)
        .def_rw("week", &oem::MetaDataStruct::usWeek)
        .def_rw("milliseconds", &oem::MetaDataStruct::dMilliseconds)
        .def_rw("binary_msg_length", &oem::MetaDataStruct::uiBinaryMsgLength,
                "Message length according to the binary header. If ASCII, this field is not used.")
        .def_rw("length", &oem::MetaDataStruct::uiLength, "Length of the entire log, including the header and CRC.")
        .def_rw("header_length", &oem::MetaDataStruct::uiHeaderLength, "The length of the message header. Used for NovAtel logs.")
        .def_rw("message_id", &oem::MetaDataStruct::usMessageId)
        .def_rw("message_crc", &oem::MetaDataStruct::uiMessageCrc)
        .def_prop_rw(
            "message_name", [](oem::MetaDataStruct& self) { return nb::str(self.acMessageName); },
            [](oem::MetaDataStruct& self, std::string message_name) {
                if (message_name.length() > oem::OEM4_ASCII_MESSAGE_NAME_MAX) throw std::runtime_error("Message name is too long");
                memcpy(self.acMessageName, message_name.c_str(), message_name.length());
                self.acMessageName[message_name.length()] = '\0';
            });

    nb::class_<oem::IntermediateHeader>(m, "IntermediateHeader")
        .def(nb::init<>())
        .def_rw("message_id", &oem::IntermediateHeader::usMessageId)
        .def_rw("message_type", &oem::IntermediateHeader::ucMessageType)
        .def_rw("port_address", &oem::IntermediateHeader::uiPortAddress)
        .def_rw("length", &oem::IntermediateHeader::usLength)
        .def_rw("sequence", &oem::IntermediateHeader::usSequence)
        .def_rw("idle_time", &oem::IntermediateHeader::ucIdleTime)
        .def_rw("time_status", &oem::IntermediateHeader::uiTimeStatus)
        .def_rw("week", &oem::IntermediateHeader::usWeek)
        .def_rw("milliseconds", &oem::IntermediateHeader::dMilliseconds)
        .def_rw("receiver_status", &oem::IntermediateHeader::uiReceiverStatus)
        .def_rw("message_definition_crc", &oem::IntermediateHeader::uiMessageDefinitionCrc)
        .def_rw("receiver_sw_version", &oem::IntermediateHeader::usReceiverSwVersion);
}