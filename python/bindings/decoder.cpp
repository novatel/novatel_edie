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
    case STATUS::SUCCESS: return create_message_instance(header, std::move(fields), metadata, database);
    case STATUS::NO_DEFINITION: return create_unknown_message_instance(raw_body, header, database);
    default: throw_exception_from_status(status);
    }
}

void init_novatel_decoder(nb::module_& m)
{
    define_pymessagedata(m);

    nb::class_<oem::PyDecoder>(m, "Decoder")
        .def(
            "__init__",
            [](oem::PyDecoder* self, PyMessageDatabase::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); }
                new (self) oem::PyDecoder(message_db);
            },
            nb::arg("message_db") = nb::none(),
            R"doc(
             Initializes a Decoder.

             Args:
                 message_db: The message database to decode messages with.
                    If None, use the default database.
            )doc")
        .def(
            "decode_header",
            [](const oem::PyDecoder& decoder, const nb::bytes raw_header, oem::MetaDataStruct* metadata) {
                oem::PyHeader header;
                if (metadata == nullptr)
                {
                    oem::MetaDataStruct default_metadata = oem::MetaDataStruct();
                    metadata = &default_metadata;
                }
                STATUS status = decoder.header_decoder.Decode(reinterpret_cast<const uint8_t*>(raw_header.c_str()), header, *metadata);
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
                header.format = metadata->eFormat;
                return header;
            },
            "raw_header"_a, nb::arg("metadata") = nb::none(),
            nb::sig("def decode_header(self, raw_header: bytes, metadata: MetaData | None = None) -> Header"),
            R"doc(
            Decode the header from a piece of framed data.

            Args:
                raw_header: A frame of raw bytes containing the header information to decode.
                metadata: A storehouse for additional information determined as part of the decoding process.
                    Supplying metadata is optional, but without it there will be no way of later accessing
                    information such as the number of bytes that make up the original header representation.

            Returns:
                A decoded `Header`.
            )doc")
        .def(
            "decode_payload",
            [](const oem::PyDecoder& decoder, const nb::bytes raw_payload, oem::PyHeader& header, oem::MetaDataStruct* metadata) {
                if (metadata == nullptr)
                {
                    oem::MetaDataStruct default_metadata = oem::MetaDataStruct();
                    default_metadata.uiHeaderLength = header.usLength;
                    default_metadata.uiBinaryMsgLength = raw_payload.size();
                    default_metadata.uiLength = default_metadata.uiHeaderLength + default_metadata.uiBinaryMsgLength;
                    default_metadata.uiMessageCrc = header.uiMessageDefinitionCrc;
                    default_metadata.eFormat = header.format;
                    default_metadata.usMessageId = header.usMessageId;
                    default_metadata.messageName = decoder.database->MsgIdToMsgName(CreateMsgId(
                        header.usMessageId, static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY), static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV), 0U));
                    default_metadata.bResponse = ((header.ucMessageType & static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE)) != 0);
                    metadata = &default_metadata;
                }
                return decoder.DecodeMessage(raw_payload, header, *metadata);
            },
            "raw_body"_a, "decoded_header"_a, nb::arg("metadata") = nb::none(),
            nb::sig("def decode_payload(self, raw_payload: bytes, header: Header, metadata: MetaData | None = None) -> Message"),
            R"doc(
            Decode the payload of a message given the associated header.

            Args:
                raw_header: A frame of raw bytes containing the payload information to decode.
                metadata: An optional way of supplying data to aid in decoding. If not provided
                    decoding will attempt to use only information from the header.

            Returns:
                A decoded `Message`.
            )doc")
        .def(
            "decode",
            [](const oem::PyDecoder& decoder, const nb::bytes raw_message) {
                oem::MetaDataStruct metadata;
                metadata.uiLength = raw_message.size();
                std::vector<FieldContainer> fields;
                oem::PyHeader header;
                const unsigned char* message_pointer = reinterpret_cast<const uint8_t*>(raw_message.c_str());
                STATUS status = decoder.header_decoder.Decode(message_pointer, header, metadata);
                header.format = metadata.eFormat;
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
                const unsigned char* body_pointer = message_pointer + metadata.uiHeaderLength;
                status = decoder.message_decoder.Decode(body_pointer, fields, metadata);
                switch (status)
                {
                case STATUS::SUCCESS: return create_message_instance(header, std::move(fields), metadata, decoder.database);
                case STATUS::NO_DEFINITION:
                    return create_unknown_message_instance(nb::bytes(body_pointer, metadata.uiLength - metadata.uiHeaderLength), header,
                                                           decoder.database);
                default: throw_exception_from_status(status);
                }
            },
            "message"_a, nb::sig("def decode(self, raw_message: bytes) -> Message"),
            R"doc(
            Decode the message from its raw byte representation.

            Args:
                raw_message: A frame of raw bytes containing the message information to decode.

            Returns:
                A decoded `Message` or an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition.
            )doc");
}
