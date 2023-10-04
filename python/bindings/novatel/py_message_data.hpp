#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

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

inline void define_pymessagedata(nb::module_& m)
{
    nb::class_<oem::PyMessageData>(m, "MessageData")
        .def_prop_ro("message", &oem::PyMessageData::message)
        .def_prop_ro("header", &oem::PyMessageData::header)
        .def_prop_ro("body", &oem::PyMessageData::body);
}
