#include "novatel_edie/decoders/common/common.hpp"

#include "bindings_core.hpp"
#include "decoder_exception.hpp"
#include "novatel_edie/version.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

novatel::edie::DecoderException::DecoderException(STATUS status_) : status(status_)
{
    auto status_py = nb::cast(status);
    std::string name = nb::cast<std::string>(repr(status_py).attr("rsplit")(".", 1)[1]);
    msg = name + ": " + nb::cast<std::string>(status_py.doc());
}

const char* novatel::edie::DecoderException::what() const noexcept { return msg.c_str(); }

void init_common_common(nb::module_& m)
{
    nb::exception<DecoderException>(m, "DecoderException");

    nb::enum_<STATUS>(m, "STATUS")
        .value("SUCCESS", STATUS::SUCCESS, "Successfully found a frame in the framer buffer.")
        .value("FAILURE", STATUS::FAILURE, "An unexpected failure occurred.")
        .value("UNKNOWN", STATUS::UNKNOWN, "Could not identify bytes as a protocol.")
        .value("INCOMPLETE", STATUS::INCOMPLETE, "It is possible that a valid frame exists in the frame buffer, but more information is needed.")
        .value("INCOMPLETE_MORE_DATA", STATUS::INCOMPLETE_MORE_DATA, "The current frame buffer is incomplete but more data is expected.")
        .value("NULL_PROVIDED", STATUS::NULL_PROVIDED, "A null pointer was provided.")
        .value("NO_DATABASE", STATUS::NO_DATABASE, "No database has been provided to the component.")
        .value("NO_DEFINITION", STATUS::NO_DEFINITION, "No definition could be found in the database for the provided message.")
        .value("NO_DEFINITION_EMBEDDED", STATUS::NO_DEFINITION_EMBEDDED,
               "No definition could be found in the database for the embedded message in the RXCONFIG log.")
        .value("BUFFER_FULL", STATUS::BUFFER_FULL, "The provided destination buffer is not big enough to contain the frame.")
        .value("BUFFER_EMPTY", STATUS::BUFFER_EMPTY, "The internal circular buffer does not contain any unread bytes")
        .value("STREAM_EMPTY", STATUS::STREAM_EMPTY, "The input stream is empty.")
        .value("UNSUPPORTED", STATUS::UNSUPPORTED, "An attempted operation is unsupported by this component.")
        .value("MALFORMED_INPUT", STATUS::MALFORMED_INPUT, "The input is recognizable, but has unexpected formatting.")
        .value("DECOMPRESSION_FAILURE", STATUS::DECOMPRESSION_FAILURE, "The RANGECMPx log could not be decompressed.");

    nb::enum_<ENCODE_FORMAT>(m, "ENCODE_FORMAT")
        .value("FLATTENED_BINARY", ENCODE_FORMAT::FLATTENED_BINARY,
               "NovAtel EDIE \"Flattened\" binary format.  All strings/arrays are padded to maximum length to allow programmatic access.")
        .value("ASCII", ENCODE_FORMAT::ASCII,
               "NovAtel ASCII. If the log was decoded from a SHORT/compressed format, it will be encoded to the respective SHORT/compressed format.")
        .value("ABBREV_ASCII", ENCODE_FORMAT::ABBREV_ASCII, "NovAtel Abbreviated ASCII.")
        .value("BINARY", ENCODE_FORMAT::BINARY,
               "NovAtel Binary. If the log was decoded from a SHORT/compressed format, it will be encoded to the respective SHORT/compressed format.")
        .value("JSON", ENCODE_FORMAT::JSON, "A JSON object.  See HTML documentation for information on fields.")
        .value("UNSPECIFIED", ENCODE_FORMAT::UNSPECIFIED, "No encode format was specified.");

    m.def("string_to_encode_format", &StringToEncodeFormat, "str"_a);

    nb::enum_<TIME_STATUS>(m, "TIME_STATUS")
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
        .value("EXACT", TIME_STATUS::EXACT, "Time is exact.");

    nb::enum_<MESSAGE_FORMAT>(m, "MESSAGE_FORMAT")
        .value("BINARY", MESSAGE_FORMAT::BINARY)
        .value("ASCII", MESSAGE_FORMAT::ASCII)
        .value("ABBREV", MESSAGE_FORMAT::ABBREV)
        .value("RSRVD", MESSAGE_FORMAT::RSRVD);

    nb::enum_<MESSAGE_TYPE_MASK>(m, "MESSAGE_TYPE_MASK")
        .value("MEASSRC", MESSAGE_TYPE_MASK::MEASSRC)
        .value("MSGFORMAT", MESSAGE_TYPE_MASK::MSGFORMAT)
        .value("RESPONSE", MESSAGE_TYPE_MASK::RESPONSE);

    nb::enum_<MESSAGE_ID_MASK>(m, "MESSAGE_ID_MASK")
        .value("LOGID", MESSAGE_ID_MASK::LOGID)
        .value("MEASSRC", MESSAGE_ID_MASK::MEASSRC)
        .value("MSGFORMAT", MESSAGE_ID_MASK::MSGFORMAT)
        .value("RESPONSE", MESSAGE_ID_MASK::RESPONSE);

    nb::enum_<MEASUREMENT_SOURCE>(m, "MEASUREMENT_SOURCE")
        .value("PRIMARY", MEASUREMENT_SOURCE::PRIMARY)
        .value("SECONDARY", MEASUREMENT_SOURCE::SECONDARY)
        .value("MAX", MEASUREMENT_SOURCE::MAX);

    nb::class_<SatelliteId>(m, "SatelliteId")
        .def(nb::init<>())
        .def_rw("prn_or_slot", &SatelliteId::usPrnOrSlot)
        .def_rw("frequency_channel", &SatelliteId::sFrequencyChannel);

    m.attr("MESSAGE_SIZE_MAX") = MESSAGE_SIZE_MAX;
    m.attr("MAX_ASCII_MESSAGE_LENGTH") = MAX_ASCII_MESSAGE_LENGTH;
    m.attr("MAX_BINARY_MESSAGE_LENGTH") = MAX_BINARY_MESSAGE_LENGTH;
    m.attr("MAX_SHORT_ASCII_MESSAGE_LENGTH") = MAX_SHORT_ASCII_MESSAGE_LENGTH;
    m.attr("MAX_SHORT_BINARY_MESSAGE_LENGTH") = MAX_SHORT_BINARY_MESSAGE_LENGTH;
    m.attr("MAX_ABB_ASCII_RESPONSE_LENGTH") = MAX_ABB_ASCII_RESPONSE_LENGTH;
    m.attr("MAX_NMEA_MESSAGE_LENGTH") = MAX_NMEA_MESSAGE_LENGTH;

    m.attr("version") = RELEASE_VERSION;
    m.attr("git_sha") = GIT_SHA;
    m.attr("git_branch") = GIT_BRANCH;
    m.attr("git_is_dirty") = GIT_IS_DIRTY;
    m.attr("build_timestamp") = BUILD_TIMESTAMP;
    m.attr("pretty_version") = caPrettyPrint;
}
