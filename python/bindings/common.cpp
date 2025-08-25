#include "novatel_edie/decoders/common/common.hpp"

#include "bindings_core.hpp"
#include "novatel_edie/version.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_common_common(nb::module_& m)
{
    nb::enum_<ENCODE_FORMAT>(m, "ENCODE_FORMAT", nb::is_arithmetic(), "Formats which a novatel message can be encoded to.")
        .value("FLATTENED_BINARY", ENCODE_FORMAT::FLATTENED_BINARY,
               "NovAtel EDIE \"Flattened\" binary format.  All strings/arrays are padded to maximum length to allow programmatic access.")
        .value("ASCII", ENCODE_FORMAT::ASCII,
               "NovAtel ASCII. If the log was decoded from a SHORT/compressed format, it will be encoded to the respective SHORT/compressed format.")
        .value("ABBREV_ASCII", ENCODE_FORMAT::ABBREV_ASCII, "NovAtel Abbreviated ASCII.")
        .value("BINARY", ENCODE_FORMAT::BINARY,
               "NovAtel Binary. If the log was decoded from a SHORT/compressed format, it will be encoded to the respective SHORT/compressed format.")
        .value("JSON", ENCODE_FORMAT::JSON, "A JSON object.  See HTML documentation for information on fields.")
        .value("UNSPECIFIED", ENCODE_FORMAT::UNSPECIFIED, "No encode format was specified.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::enum_<TIME_STATUS>(m, "TIME_STATUS", nb::is_arithmetic(), "Indications of how well a GPS reference time is known.")
        .value("UNKNOWN", TIME_STATUS::UNKNOWN, "Time validity is unknown.")
        .value("APPROXIMATE", TIME_STATUS::APPROXIMATE, "Time is set approximately.")
        .value("COARSEADJUSTING", TIME_STATUS::COARSEADJUSTING, "Time is approaching coarse precision.")
        .value("COARSE", TIME_STATUS::COARSE, "This time is valid to coarse precision.")
        .value("COARSESTEERING", TIME_STATUS::COARSESTEERING, "Time is coarse set and is being steered.")
        .value("FREEWHEELING", TIME_STATUS::FREEWHEELING, "Position is lost and the range bias cannot be calculated.")
        .value("FINEADJUSTING", TIME_STATUS::FINEADJUSTING, "Time is adjusting to fine precision.")
        .value("FINE", TIME_STATUS::FINE, "Time has fine precision.")
        .value("FINEBACKUPSTEERING", TIME_STATUS::FINEBACKUPSTEERING, "Time is fine set and is being steered by the backup system.")
        .value("FINESTEERING", TIME_STATUS::FINESTEERING, "Time is fine set and is being steered.")
        .value("SATTIME", TIME_STATUS::SATTIME, "Time from satellite. Only used in logs containing satellite data such as ephemeris and almanac.")
        .value("EXTERN", TIME_STATUS::EXTERN, "Time source is external to the Receiver.")
        .value("EXACT", TIME_STATUS::EXACT, "Time is exact.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::enum_<MESSAGE_FORMAT>(m, "MESSAGE_FORMAT", nb::is_arithmetic(), "Message formats supported natively by a novatel reciever.")
        .value("BINARY", MESSAGE_FORMAT::BINARY, "Binary format.")
        .value("ASCII", MESSAGE_FORMAT::ASCII, "ASCII format.")
        .value("ABBREV", MESSAGE_FORMAT::ABBREV, "Abbreviated ASCII format.")
        .value("RSRVD", MESSAGE_FORMAT::RSRVD, "Format reserved for future use.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::enum_<MESSAGE_TYPE_MASK>(
        m, "MESSAGE_TYPE_MASK", nb::is_arithmetic(),
        "Bitmasks for extracting data from binary `message type` fields which appear within novatel headers and certain logs.")
        .value("MEASSRC", MESSAGE_TYPE_MASK::MEASSRC, "Bitmask for extracting the source of message.")
        .value("MSGFORMAT", MESSAGE_TYPE_MASK::MSGFORMAT, "Bitmask for extracting the format of a message.")
        .value("RESPONSE", MESSAGE_TYPE_MASK::RESPONSE, "Bitmask for extracting the response status of a message.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::enum_<MESSAGE_ID_MASK>(
        m, "MESSAGE_ID_MASK", nb::is_arithmetic(),
        "Bitmasks for extracting data from `message id` and `message type` fields which appear within novatel headers and certain logs.")
        .value("LOGID", MESSAGE_ID_MASK::LOGID, "Bitmask for extracting the id of message/log.")
        .value("MEASSRC", MESSAGE_ID_MASK::MEASSRC, "Bitmask for extracting the source of message.")
        .value("MSGFORMAT", MESSAGE_ID_MASK::MSGFORMAT, "Bitmask for extracting the format of a message.")
        .value("RESPONSE", MESSAGE_ID_MASK::RESPONSE, "Bitmask for extracting the response status of a message.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::enum_<MEASUREMENT_SOURCE>(m, "MEASUREMENT_SOURCE", nb::is_arithmetic(), "Origins for a message.")
        .value("PRIMARY", MEASUREMENT_SOURCE::PRIMARY, "Originates from primary antenna.")
        .value("SECONDARY", MEASUREMENT_SOURCE::SECONDARY, "Originates from secondary antenna.")
        .value("MAX", MEASUREMENT_SOURCE::MAX)
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::class_<SatelliteId>(m, "SatelliteId")
        .def(nb::init())
        .def("to_dict",
             [](SatelliteId& self) {
                 nb::dict dict;
                 dict["prn"] = self.usPrnOrSlot;
                 dict["frequency_channel"] = self.sFrequencyChannel;
                 return dict;
             })
        .def_rw("prn", &SatelliteId::usPrnOrSlot, "The satellite PRN for GPS or the slot for GLONASS.")
        .def_rw("prn_or_slot", &SatelliteId::usPrnOrSlot, "DEPRECATED: Use 'prn' field instead.")
        .def_rw("frequency_channel", &SatelliteId::sFrequencyChannel, "The frequency channel if it is a GLONASS satilite, otherwise left as zero.")
        .def("__repr__", [](SatelliteId id) {
            return nb::str("SatelliteId(prn={!r}, frequency_channel={!r})").format(id.usPrnOrSlot, id.sFrequencyChannel);
        });

    m.attr("MAX_MESSAGE_LENGTH") = MESSAGE_SIZE_MAX;
    m.attr("MAX_ASCII_MESSAGE_LENGTH") = MAX_ASCII_MESSAGE_LENGTH;
    m.attr("MAX_BINARY_MESSAGE_LENGTH") = MAX_BINARY_MESSAGE_LENGTH;
    m.attr("MAX_SHORT_ASCII_MESSAGE_LENGTH") = MAX_SHORT_ASCII_MESSAGE_LENGTH;
    m.attr("MAX_SHORT_BINARY_MESSAGE_LENGTH") = MAX_SHORT_BINARY_MESSAGE_LENGTH;
    m.attr("MAX_ABB_ASCII_RESPONSE_LENGTH") = MAX_ABB_ASCII_RESPONSE_LENGTH;
    m.attr("MAX_NMEA_MESSAGE_LENGTH") = MAX_NMEA_MESSAGE_LENGTH;

    m.attr("CPP_VERSION") = RELEASE_VERSION;
    m.attr("GIT_SHA") = GIT_SHA;
    m.attr("GIT_BRANCH") = GIT_BRANCH;
    m.attr("GIT_IS_DIRTY") = GIT_IS_DIRTY;
    m.attr("BUILD_TIMESTAMP") = BUILD_TIMESTAMP;
    m.attr("CPP_PRETTY_VERSION") = caPrettyPrint;

    m.def("calculate_crc", [](nb::bytes bytes) { return CalculateBlockCrc32(reinterpret_cast<const unsigned char*>(bytes.c_str()), bytes.size()); });
}
