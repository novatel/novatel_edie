#include "novatel_edie/decoders/oem/header_decoder.hpp"

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "py_decoded_message.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;

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

void init_novatel_header_decoder(nb::module_& m)
{
    nb::class_<oem::PyHeader>(m, "Header")
        .def_ro("message_id", &oem::PyHeader::usMessageId)
        .def_ro("message_type", &oem::PyHeader::ucMessageType)
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

    nb::class_<oem::HeaderDecoder>(m, "HeaderDecoder")
        .def("__init__", [](oem::HeaderDecoder* t) { new (t) oem::HeaderDecoder(MessageDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "json_db"_a)
        .def("load_json_db", &oem::HeaderDecoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", [](oem::HeaderDecoder& decoder) { return decoder.GetLogger(); })
        .def(
            "decode",
            [](const oem::HeaderDecoder& decoder, const nb::bytes& raw_header, oem::MetaDataStruct& metadata) {
                oem::PyHeader header;
                STATUS status = decoder.Decode(reinterpret_cast<const uint8_t*>(raw_header.c_str()), header, metadata);
                return nb::make_tuple(status, header);
            },
            "header"_a, "metadata"_a);
}
