#pragma once

#include "novatel_edie/decoders/common/common.hpp"
#include "py_common/bindings_core.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_common {

struct PyMessageData
{
    PyMessageData(MessageDataStruct message_data)
        : message_((char*)message_data.pucMessage, message_data.uiMessageLength),
          header_offset(message_data.pucMessageHeader - message_data.pucMessage), header_size(message_data.uiMessageHeaderLength),
          body_offset(message_data.pucMessageBody - message_data.pucMessage), body_size(message_data.uiMessageBodyLength)
    {
    }

    [[nodiscard]] const nb::bytes& message() const { return message_; }

    [[nodiscard]] nb::object header() const { return message_[nb::slice(header_offset, header_offset + header_size)]; }

    [[nodiscard]] nb::object body() const { return message_[nb::slice(body_offset, body_offset + body_size)]; }

    std::string GetRepr() { return "MessageData(" + std::string(nb::cast<nb::str>(message_.attr("__repr__")()).c_str()) + ")"; }

  private:
    const nb::bytes message_;
    const uint32_t header_offset;
    const uint32_t header_size;
    const uint32_t body_offset;
    const uint32_t body_size;
};

} // namespace novatel::edie::py_common
