#include "py_oem/decoder.hpp"

#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_common/py_message_data.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

nb::object py_oem::PyDecoder::DecodeMessage(std::string_view raw_body, py_oem::PyHeader& header, oem::MetaDataStruct& metadata) const
{
    CompositeField fields;
    STATUS status;

    if (metadata.eFormat == DECODE_FORMAT::BINARY)
    {
        if (metadata.uiBinaryMsgLength > static_cast<uint64_t>(raw_body.size()))
        {
            throw std::runtime_error("Provided binary message length exceeds available raw body size.");
        }
    }
    else
    {
        // For ASCII, MessageDecoder::Decode internally subtracts the header length from the total length, so
        // the number of bytes actually consumed by the decoder is at most metadata.uiLength - metadata.uiHeaderLength.
        if (metadata.uiLength > static_cast<uint64_t>(metadata.uiHeaderLength) + raw_body.size())
        {
            throw std::runtime_error("Provided metadata length exceeds available length.");
        }
        if (metadata.uiLength > raw_body.size() && metadata.eFormat == DECODE_FORMAT::JSON)
        {
            throw std::runtime_error("Provided metadata length exceeds buffer size for JSON format.");
        }
        if (metadata.uiHeaderLength > metadata.uiLength) { throw std::runtime_error("Header length exceeds total metadata length."); }
    }

    if (rx_config_handler.IsRxConfigTypeMsg(header.usMessageId))
    {
        status = rx_config_handler.Decode(reinterpret_cast<const unsigned char*>(raw_body.data()), fields, metadata);
    }
    else
    {
        status = message_decoder.Decode(reinterpret_cast<const unsigned char*>(raw_body.data()), fields, metadata);
    }

    switch (status)
    {
    case STATUS::SUCCESS: return py_oem::create_message_instance(header, std::move(fields), metadata, database);
    case STATUS::NO_DEFINITION: return py_oem::create_unknown_message_instance(nb::bytes(raw_body.data(), raw_body.size()), header);
    default: throw_exception_from_failing_status(status);
    }
}

void py_oem::init_novatel_decoder(nb::module_& m)
{
    nb::class_<py_oem::PyDecoder>(m, "Decoder")
        .def(
            "__init__",
            [](py_oem::PyDecoder* self, py_common::PyMessageDatabase::Ptr message_db) {
                if (!message_db) { message_db = py_oem::MessageDbSingleton::get(); }
                message_db->Lock();
                new (self) py_oem::PyDecoder(message_db);
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
            [](const py_oem::PyDecoder& decoder, const nb::bytes raw_header, oem::MetaDataStruct* metadata) {
                py_oem::PyHeader header;
                oem::MetaDataStruct default_metadata;
                if (metadata == nullptr)
                {
                    metadata = &default_metadata;
                    metadata->uiLength = static_cast<uint32_t>(raw_header.size());
                }

                // At this point, metadata->uiLength could either be a full frame length (e.g. as set by the framer),
                // or just the header length. Also, raw_header could contain the full message frame or just a header.
                // Since HeaderDecoder bounds the buffer by the provided metadata length, the safest thing to do is
                // to set metadata->uiLength to the size of raw_header.
                metadata->uiLength = raw_header.size();
                STATUS status = decoder.header_decoder.Decode(reinterpret_cast<const unsigned char*>(raw_header.c_str()), header, *metadata);
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
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
                    If supplied, its length is set to len(raw_header).

            Returns:
                A decoded `Header`.
            )doc")
        .def(
            "decode_payload",
            [](const py_oem::PyDecoder& decoder, const nb::bytes raw_payload, py_oem::PyHeader& header, oem::MetaDataStruct* metadata) {
                oem::MetaDataStruct default_metadata;
                if (metadata == nullptr)
                {
                    default_metadata.uiBinaryMsgLength = static_cast<uint32_t>(raw_payload.size());
                    default_metadata.uiLength = default_metadata.uiHeaderLength + default_metadata.uiBinaryMsgLength;
                    default_metadata.uiMessageCrc = header.uiMessageDefinitionCrc;
                    default_metadata.usMessageId = header.usMessageId;
                    default_metadata.messageName = decoder.database->MsgIdToMsgName(
                        CreateMsgId(header.usMessageId, NULL_SIBLING_ID, static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV), 0U));
                    default_metadata.bResponse = ((header.ucMessageType & static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE)) != 0);
                    metadata = &default_metadata;
                }
                metadata->uiLength = metadata->uiHeaderLength + raw_payload.size();
                return decoder.DecodeMessage(std::string_view(raw_payload.c_str(), raw_payload.size()), header, *metadata);
            },
            "raw_body"_a, "decoded_header"_a, nb::arg("metadata") = nb::none(),
            nb::sig("def decode_payload(self, raw_body: bytes, decoded_header: Header, metadata: MetaData | None = None) -> Message | "
                    "UnknownMessage"),
            R"doc(
            Decode the payload of a message given the associated header.

            Args:
                raw_body: The bytes following the header, including any CRC or trailer. For JSON, the
                    whole document containing "body".
                decoded_header: The already-decoded header describing the payload.
                metadata: Metadata from decode_header, with format matching the input. Its length is set
                    to header_length + len(raw_body). If omitted, the format is unknown, so a known message
                    cannot be decoded.

            Returns:
                A decoded `Message`, or an `UnknownMessage` if no definition was available
                for the message.

            Raises:
                RuntimeError: If metadata.binary_msg_length exceeds len(raw_body) for binary input.
            )doc")
        .def(
            "decode",
            [](const py_oem::PyDecoder& decoder, const nb::bytes raw_message) {
                oem::MetaDataStruct metadata;
                metadata.uiLength = static_cast<uint32_t>(raw_message.size());
                py_oem::PyHeader header;
                const unsigned char* message_pointer = reinterpret_cast<const unsigned char*>(raw_message.c_str());
                STATUS status = decoder.header_decoder.Decode(message_pointer, header, metadata);
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
                return decoder.DecodeMessage(std::string_view(raw_message.c_str() + metadata.uiHeaderLength,
                                                              metadata.uiLength - metadata.uiHeaderLength),
                                             header, metadata);
            },
            "message"_a, nb::sig("def decode(self, message: bytes) -> Message | Response | UnknownMessage"),
            R"doc(
            Decode the message from its raw byte representation.

            Args:
                message: A single complete message frame, starting at its first sync byte.

            Returns:
                A decoded `Message`, a `Response`, or an `UnknownMessage` whose header was identified
                but whose payload could not be decoded due to no available message definition.
            )doc");
}
