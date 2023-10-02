#include "novatel_edie/decoders/oem/encoder.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

namespace novatel::edie::oem {
struct PyMessageData
{
    PyMessageData(const MessageDataStruct& message_data)
        : message_((char*)message_data.pucMessage, message_data.uiMessageLength),
          header_offset(message_data.pucMessageHeader - message_data.pucMessage), header_size(message_data.uiMessageHeaderLength),
          body_offset(message_data.pucMessageBody - message_data.pucMessage), body_size(message_data.uiMessageBodyLength)
    {
    }

    const nb::bytes& message() const { return message_; }

    nb::object header() const { return message_[nb::slice(header_offset, header_offset + header_size)]; }

    nb::object body() const { return message_[nb::slice(body_offset, body_offset + body_size)]; }

  private:
    const nb::bytes message_;
    const uint32_t header_offset;
    const uint32_t header_size;
    const uint32_t body_offset;
    const uint32_t body_size;
};
} // namespace novatel::edie::oem

void init_novatel_encoder(nb::module_& m)
{

    nb::class_<oem::PyMessageData>(m, "MessageData")
        .def_prop_ro("message", &oem::PyMessageData::message)
        .def_prop_ro("header", &oem::PyMessageData::header)
        .def_prop_ro("body", &oem::PyMessageData::body);

    nb::class_<oem::Encoder>(m, "Encoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("open", &oem::Encoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::Encoder::GetLogger)
        .def(
            "encode",
            [](oem::Encoder& encoder, oem::IntermediateHeader& header, IntermediateMessage& message, oem::MetaDataStruct& metadata,
               ENCODE_FORMAT format) {
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                novatel::edie::MessageDataStruct message_data;
                STATUS status = encoder.Encode((unsigned char**)&buffer, buf_size, header, message, message_data, metadata, format);
                return nb::make_tuple(status, oem::PyMessageData(message_data));
            },
            "header"_a, "message"_a, "metadata"_a, "encode_format"_a);
}
