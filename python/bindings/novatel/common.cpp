#include "novatel_edie/decoders/oem/common.hpp"

#include "bindings_core.hpp"
#include "json_db_singleton.hpp"
#include "novatel_edie/common/nexcept.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

namespace {
std::string default_json_db_path()
{
    nb::object ir = nb::module_::import_("importlib_resources");
    nb::object path_ctx = ir.attr("as_file")(ir.attr("files")("novatel_edie").attr("joinpath")("messages_public.json"));
    auto py_path = path_ctx.attr("__enter__")();
    if (!nb::cast<bool>(py_path.attr("is_file")()))
        throw NExcept((std::string("Could not find the default JSON DB file at ") + nb::str(py_path).c_str()).c_str());
    auto path = nb::cast<std::string>(nb::str(py_path));
    path_ctx.attr("__exit__")(nb::none(), nb::none(), nb::none());
    return path;
}
} // namespace

JsonReader* JsonDbSingleton::get()
{
    static std::unique_ptr<JsonReader> json_db = nullptr;
    if (!json_db)
    {
        // Using a temp variable to avoid an inconsistent state if LoadFile throws
        auto db = std::make_unique<JsonReader>();
        db->LoadFile(default_json_db_path());
        json_db = std::move(db);
    }
    return json_db.get();
}

void init_novatel_common(nb::module_& m)
{
    m.attr("NMEA_SYNC") = oem::NMEA_SYNC;
    m.attr("NMEA_SYNC_LENGTH") = oem::NMEA_SYNC_LENGTH;
    m.attr("NMEA_CRC_LENGTH") = oem::NMEA_CRC_LENGTH;
    m.attr("OEM4_ASCII_SYNC") = oem::OEM4_ASCII_SYNC;
    m.attr("OEM4_ASCII_FIELD_SEPARATOR") = oem::OEM4_ASCII_FIELD_SEPARATOR;
    m.attr("OEM4_ASCII_HEADER_TERMINATOR") = oem::OEM4_ASCII_HEADER_TERMINATOR;
    m.attr("OEM4_ASCII_SYNC_LENGTH") = oem::OEM4_ASCII_SYNC_LENGTH;
    m.attr("OEM4_ASCII_CRC_DELIMITER") = oem::OEM4_ASCII_CRC_DELIMITER;
    m.attr("OEM4_ASCII_CRC_LENGTH") = oem::OEM4_ASCII_CRC_LENGTH;
    m.attr("OEM4_SHORT_ASCII_SYNC") = oem::OEM4_SHORT_ASCII_SYNC;
    m.attr("OEM4_ASCII_MESSAGE_NAME_MAX") = oem::OEM4_ASCII_MESSAGE_NAME_MAX;
    m.attr("OEM4_SHORT_ASCII_SYNC_LENGTH") = oem::OEM4_SHORT_ASCII_SYNC_LENGTH;
    m.attr("OEM4_ABBREV_ASCII_SYNC") = oem::OEM4_ABBREV_ASCII_SYNC;
    m.attr("OEM4_ABBREV_ASCII_SEPARATOR") = oem::OEM4_ABBREV_ASCII_SEPARATOR;
    m.attr("OEM4_ABBREV_ASCII_INDENTATION_LENGTH") = oem::OEM4_ABBREV_ASCII_INDENTATION_LENGTH;
    m.attr("OEM4_ERROR_PREFIX_LENGTH") = oem::OEM4_ERROR_PREFIX_LENGTH;
    m.attr("OEM4_BINARY_SYNC1") = oem::OEM4_BINARY_SYNC1;
    m.attr("OEM4_BINARY_SYNC2") = oem::OEM4_BINARY_SYNC2;
    m.attr("OEM4_BINARY_SYNC3") = oem::OEM4_BINARY_SYNC3;
    m.attr("OEM4_BINARY_SYNC_LENGTH") = oem::OEM4_BINARY_SYNC_LENGTH;
    m.attr("OEM4_BINARY_HEADER_LENGTH") = oem::OEM4_BINARY_HEADER_LENGTH;
    m.attr("OEM4_BINARY_CRC_LENGTH") = oem::OEM4_BINARY_CRC_LENGTH;
    m.attr("OEM4_SHORT_BINARY_SYNC3") = oem::OEM4_SHORT_BINARY_SYNC3;
    m.attr("OEM4_SHORT_BINARY_SYNC_LENGTH") = oem::OEM4_SHORT_BINARY_SYNC_LENGTH;
    m.attr("OEM4_SHORT_BINARY_HEADER_LENGTH") = oem::OEM4_SHORT_BINARY_HEADER_LENGTH;
    m.attr("OEM4_PROPRIETARY_BINARY_SYNC2") = oem::OEM4_PROPRIETARY_BINARY_SYNC2;

    nb::enum_<oem::ASCII_HEADER>(m, "ASCII_HEADER", "ASCII Message header format sequence")
        .value("MESSAGE_NAME", oem::ASCII_HEADER::MESSAGE_NAME, "ASCII log Name.")
        .value("PORT", oem::ASCII_HEADER::PORT, "Receiver logging port.")
        .value("SEQUENCE", oem::ASCII_HEADER::SEQUENCE, "Embedded log sequence number.")
        .value("IDLE_TIME", oem::ASCII_HEADER::IDLE_TIME, "Receiver Idle time.")
        .value("TIME_STATUS", oem::ASCII_HEADER::TIME_STATUS, "GPS reference time status.")
        .value("WEEK", oem::ASCII_HEADER::WEEK, "GPS Week number.")
        .value("SECONDS", oem::ASCII_HEADER::SECONDS, "GPS week seconds.")
        .value("RECEIVER_STATUS", oem::ASCII_HEADER::RECEIVER_STATUS, "Receiver status.")
        .value("MSG_DEF_CRC", oem::ASCII_HEADER::MSG_DEF_CRC, "Reserved Field.")
        .value("RECEIVER_SW_VERSION", oem::ASCII_HEADER::RECEIVER_SW_VERSION, "Receiver software version.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

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
        .value("ALL", novatel::edie::HEADER_FORMAT::ALL)
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::class_<oem::MetaDataStruct>(m, "MetaData")
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
            })
        .def("__repr__", [](nb::handle self) {
            auto& metadata = nb::cast<oem::MetaDataStruct&>(self);
            return nb::str("MetaData(message_name={!r}, format={!r}, measurement_source={!r}, time_status={!r}, response={!r}, "
                           "week={!r}, milliseconds={!r}, binary_msg_length={!r}, length={!r}, header_length={!r}, message_id={!r}, "
                           "message_crc={!r})")
                .format(metadata.acMessageName, metadata.eFormat, metadata.eMeasurementSource, metadata.eTimeStatus, metadata.bResponse,
                        metadata.usWeek, metadata.dMilliseconds, metadata.uiBinaryMsgLength, metadata.uiLength, metadata.uiHeaderLength,
                        metadata.usMessageId, metadata.uiMessageCrc);
        });

    nb::class_<oem::IntermediateHeader>(m, "Header")
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
        .def_rw("receiver_sw_version", &oem::IntermediateHeader::usReceiverSwVersion)
        .def(
            "get_msg_def", [](oem::IntermediateHeader& self) { return JsonDbSingleton::get()->GetMsgDef(self.usMessageId); },
            "Gets the message definition for the header's message ID from the default database")
        .def("__repr__", [](nb::handle self) {
            auto& header = nb::cast<oem::IntermediateHeader&>(self);
            auto* msg_def = JsonDbSingleton::get()->GetMsgDef(header.usMessageId);
            const std::string& msg_name = msg_def ? msg_def->name : "Unknown";
            return nb::str("Header(message_id=<{}: {}>, message_type={!r}, port_address={!r}, length={!r}, sequence={!r}, "
                           "idle_time={!r}, time_status={}, week={!r}, milliseconds={!r}, receiver_status={!r}, "
                           "message_definition_crc={!r}, receiver_sw_version={!r})")
                .format(msg_name, header.usMessageId, header.ucMessageType, header.uiPortAddress, header.usLength, header.usSequence,
                        header.ucIdleTime, TIME_STATUS(header.uiTimeStatus), header.usWeek, header.dMilliseconds, header.uiReceiverStatus,
                        header.uiMessageDefinitionCrc, header.usReceiverSwVersion);
        });

    nb::class_<oem::Oem4BinaryHeader>(m, "Oem4BinaryHeader")
        .def(nb::init<>())
        .def("__init__",
             [](oem::Oem4BinaryHeader* t, nb::bytes header_data) {
                 if (nb::len(header_data) < sizeof(oem::Oem4BinaryHeader))
                 {
                     throw nb::value_error(nb::str("Invalid header data length: {} instead of {} bytes")
                                               .format(nb::len(header_data), sizeof(oem::Oem4BinaryHeader))
                                               .c_str());
                 }
                 auto* header = new (t) oem::Oem4BinaryHeader();
                 memcpy(header, header_data.c_str(), sizeof(oem::Oem4BinaryHeader));
             })
        .def_rw("sync1", &oem::Oem4BinaryHeader::ucSync1, "First sync byte of Header.")
        .def_rw("sync2", &oem::Oem4BinaryHeader::ucSync2, "Second sync byte of Header.")
        .def_rw("sync3", &oem::Oem4BinaryHeader::ucSync3, "Third sync byte of Header.")
        .def_rw("header_length", &oem::Oem4BinaryHeader::ucHeaderLength, "Total Binary header length.")
        .def_rw("msg_number", &oem::Oem4BinaryHeader::usMsgNumber, "Binary log Message Number/ID.")
        .def_rw("msg_type", &oem::Oem4BinaryHeader::ucMsgType, "Binary log Message type response or data?.")
        .def_rw("port", &oem::Oem4BinaryHeader::ucPort, "Receiver Port of logging.")
        .def_rw("length", &oem::Oem4BinaryHeader::usLength, "Total length of binary log.")
        .def_rw("sequence_number", &oem::Oem4BinaryHeader::usSequenceNumber, "Sequence number of Embedded message inside.")
        .def_rw("idle_time", &oem::Oem4BinaryHeader::ucIdleTime, "Receiver Idle time.")
        .def_rw("time_status", &oem::Oem4BinaryHeader::ucTimeStatus, "GPS reference time status.")
        .def_rw("week_no", &oem::Oem4BinaryHeader::usWeekNo, "GPS Week number.")
        .def_rw("week_msec", &oem::Oem4BinaryHeader::uiWeekMSec, "GPS week seconds.")
        .def_rw("status", &oem::Oem4BinaryHeader::uiStatus, "Status of the log.")
        .def_rw("msg_def_crc", &oem::Oem4BinaryHeader::usMsgDefCrc, "Message def CRC of binary log.")
        .def_rw("receiver_sw_version", &oem::Oem4BinaryHeader::usReceiverSwVersion, "Receiver Software version.")
        .def("__bytes__", [](oem::Oem4BinaryHeader& self) { return nb::bytes((char*)&self, sizeof(oem::Oem4BinaryHeader)); })
        .def("__repr__", [](oem::Oem4BinaryHeader& self) {
            return nb::str("Oem4BinaryHeader(sync1={!r}, sync2={!r}, sync3={!r}, header_length={!r}, msg_number={!r}, "
                           "msg_type={!r}, port={!r}, length={!r}, sequence_number={!r}, idle_time={!r}, time_status={}, "
                           "week_no={!r}, week_milliseconds={!r}, status={!r}, msg_def_crc={!r}, receiver_sw_version={!r})")
                .format(self.ucSync1, self.ucSync2, self.ucSync3, self.ucHeaderLength, self.usMsgNumber, self.ucMsgType, self.ucPort, self.usLength,
                        self.usSequenceNumber, self.ucIdleTime, TIME_STATUS(self.ucTimeStatus), self.usWeekNo, self.uiWeekMSec, self.uiStatus,
                        self.usMsgDefCrc, self.usReceiverSwVersion);
        });

    nb::class_<oem::Oem4BinaryShortHeader>(m, "Oem4BinaryShortHeader")
        .def(nb::init<>())
        .def("__init__",
             [](oem::Oem4BinaryShortHeader* t, nb::bytes header_data) {
                 if (nb::len(header_data) != sizeof(oem::Oem4BinaryShortHeader))
                 {
                     throw nb::value_error(nb::str("Invalid header data length: {} instead of {}")
                                               .format(nb::len(header_data), sizeof(oem::Oem4BinaryShortHeader))
                                               .c_str());
                 }
                 auto* header = new (t) oem::Oem4BinaryShortHeader();
                 memcpy(header, header_data.c_str(), sizeof(oem::Oem4BinaryShortHeader));
             })
        .def_rw("sync1", &oem::Oem4BinaryShortHeader::ucSync1, "First sync byte of Header.")
        .def_rw("sync2", &oem::Oem4BinaryShortHeader::ucSync2, "Second sync byte of Header.")
        .def_rw("sync3", &oem::Oem4BinaryShortHeader::ucSync3, "Third sync byte of Header.")
        .def_rw("length", &oem::Oem4BinaryShortHeader::ucLength, "Message body length.")
        .def_rw("message_id", &oem::Oem4BinaryShortHeader::usMessageId, "Message ID of the log.")
        .def_rw("week_no", &oem::Oem4BinaryShortHeader::usWeekNo, "GPS Week number.")
        .def_rw("week_msec", &oem::Oem4BinaryShortHeader::uiWeekMSec, "GPS Week seconds.")
        .def("__bytes__", [](oem::Oem4BinaryShortHeader& self) { return nb::bytes((char*)&self, sizeof(oem::Oem4BinaryShortHeader)); })
        .def("__repr__", [](oem::Oem4BinaryShortHeader& self) {
            return nb::str("Oem4BinaryShortHeader(sync1={!r}, sync2={!r}, sync3={!r}, length={!r}, message_id={!r}, "
                           "week_no={!r}, week_msec={!r})")
                .format(self.ucSync1, self.ucSync2, self.ucSync3, self.ucLength, self.usMessageId, self.usWeekNo, self.uiWeekMSec);
        });

    m.def("get_default_database", &JsonDbSingleton::get, "Get the default JSON database singleton");
}
