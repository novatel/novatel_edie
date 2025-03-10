#include "decoder.hpp"

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "message_db_singleton.hpp"
#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

nb::object oem::PyDecoder::DecodeMessage(const nb::bytes raw_body, oem::PyHeader& header, oem::MetaDataStruct& metadata) const
{
    std::vector<FieldContainer> fields;
    STATUS status = message_decoder.Decode(reinterpret_cast<const uint8_t*>(raw_body.c_str()), fields, metadata);

    switch (status)
    {
    case STATUS::SUCCESS: return create_message_instance(header, fields, metadata, database);
    case STATUS::NO_DEFINITION: return create_unknown_message_instance(raw_body, header, database);
    default: throw_exception_from_status(status);
    }
}

void init_novatel_decoder(nb::module_& m)
{
    define_pymessagedata(m);

    nb::class_<oem::PyDecoder>(m, "Decoder")
        .def(nb::init<>())
        .def(nb::init<PyMessageDatabase::Ptr&>(), "message_db"_a)
        .def_prop_ro("header_logger", [](oem::PyDecoder& self) { return self.header_decoder.GetLogger(); })
        .def_prop_ro("message_logger", [](oem::PyDecoder& self) { return self.message_decoder.GetLogger(); })
        .def(
            "decode_header",
            [](const oem::PyDecoder& decoder, const nb::bytes raw_header, oem::MetaDataStruct& metadata) {
                oem::PyHeader header;
                STATUS status = decoder.header_decoder.Decode(reinterpret_cast<const uint8_t*>(raw_header.c_str()), header, metadata);
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
                header.format = metadata.eFormat;
                return header;
            },
            "raw_header"_a, "metadata"_a = oem::MetaDataStruct())
        .def("decode_message", &oem::PyDecoder::DecodeMessage, "raw_body"_a, "decoded_header"_a, "metadata"_a)
        .def(
            "decode_message",
            [](const oem::PyDecoder& decoder, const nb::bytes raw_body, oem::PyHeader& header) {
                oem::MetaDataStruct metadata;
                metadata.uiHeaderLength = header.usLength;
                metadata.uiBinaryMsgLength = raw_body.size();
                metadata.uiLength = metadata.uiHeaderLength + metadata.uiBinaryMsgLength;
                metadata.uiMessageCrc = header.uiMessageDefinitionCrc;
                metadata.eFormat = header.format;
                metadata.usMessageId = header.usMessageId;
                metadata.messageName = decoder.database->MsgIdToMsgName(CreateMsgId(
                    header.usMessageId, static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY), static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV), 0U));
                metadata.bResponse = header.GetPyMessageType().IsResponse();
                return decoder.DecodeMessage(raw_body, header, metadata);
            },
            "raw_body"_a, "decoded_header"_a)
        .def(
            "decode",
            [](const oem::PyDecoder& decoder, const nb::bytes message) {
                oem::MetaDataStruct metadata;
                metadata.uiLength = message.size();
                std::vector<FieldContainer> fields;
                oem::PyHeader header;
                const unsigned char* message_pointer = reinterpret_cast<const uint8_t*>(message.c_str());
                STATUS status = decoder.header_decoder.Decode(message_pointer, header, metadata);
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
                const unsigned char* body_pointer = message_pointer + metadata.uiHeaderLength;
                status = decoder.message_decoder.Decode(body_pointer, fields, metadata);
                switch (status)
                {
                case STATUS::SUCCESS: return create_message_instance(header, fields, metadata, decoder.database);
                case STATUS::NO_DEFINITION:
                    return create_unknown_message_instance(nb::bytes(body_pointer, metadata.uiLength - metadata.uiHeaderLength), header,
                                                           decoder.database);
                default: throw_exception_from_status(status);
                }
            },
            "message"_a);
}
