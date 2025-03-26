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
                if (metadata == nullptr) { 
                    oem::MetaDataStruct default_metadata = oem::MetaDataStruct();
                    metadata = &default_metadata;
                }
                STATUS status = decoder.header_decoder.Decode(reinterpret_cast<const uint8_t*>(raw_header.c_str()), header, *metadata);
                if (status != STATUS::SUCCESS) { throw_exception_from_status(status); }
                header.format = metadata->eFormat;
                return header;
            },
            "raw_header"_a, nb::arg("metadata") = nb::none())
        .def(
            "decode_message",
            [](const oem::PyDecoder& decoder, const nb::bytes raw_body, oem::PyHeader& header, oem::MetaDataStruct* metadata) {
                if (metadata == nullptr) { 
                    oem::MetaDataStruct default_metadata = oem::MetaDataStruct();
                    default_metadata.uiHeaderLength = header.usLength;
                    default_metadata.uiBinaryMsgLength = raw_body.size();
                    default_metadata.uiLength = default_metadata.uiHeaderLength + default_metadata.uiBinaryMsgLength;
                    default_metadata.uiMessageCrc = header.uiMessageDefinitionCrc;
                    default_metadata.eFormat = header.format;
                    default_metadata.usMessageId = header.usMessageId;
                    default_metadata.messageName = decoder.database->MsgIdToMsgName(CreateMsgId(
                        header.usMessageId, static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY), static_cast<uint32_t>(MESSAGE_FORMAT::ABBREV), 0U));
                    default_metadata.bResponse = header.GetPyMessageType().IsResponse();
                    metadata = &default_metadata;
                }
                return decoder.DecodeMessage(raw_body, header, *metadata);
            },
            "raw_body"_a, "decoded_header"_a, nb::arg("metadata") = nb::none())
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
