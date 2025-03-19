#include "novatel_edie/decoders/oem/message_decoder.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "../bindings_core.hpp"
#include "../message_db_singleton.hpp"
#include "../py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;

// For unit tests
class DecoderTester : public oem::MessageDecoder
{
  public:
    using MessageDecoder::MessageDecoder;

    [[nodiscard]] STATUS TestDecodeBinary(const std::vector<BaseField::Ptr>& MsgDefFields_, const uint8_t** ppucLogBuf_,
                                          std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_) const
    {
        return DecodeBinary(MsgDefFields_, ppucLogBuf_, vIntermediateFormat_, uiMessageLength_);
    }

    [[nodiscard]] STATUS TestDecodeAscii(const std::vector<BaseField::Ptr>& MsgDefFields_, const char** ppcLogBuf_,
                                         std::vector<FieldContainer>& vIntermediateFormat_) const
    {
        return DecodeAscii<false>(MsgDefFields_, ppcLogBuf_, vIntermediateFormat_);
    }
};

void init_decoder_tester(nb::module_ &m)
{
    nb::class_<DecoderTester>(m, "DecoderTester")
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "json_db"_a)
        .def(
            "decode_ascii",
            [](DecoderTester& decoder, const std::vector<BaseField::Ptr>& msg_def_fields, const nb::bytes& message_body) {
                std::vector<FieldContainer> fields;
                // Copy to ensure that the byte string is zero-delimited
                std::string body_str(message_body.c_str(), message_body.size());
                const char* data_ptr = body_str.c_str();
                STATUS status = decoder.TestDecodeAscii(msg_def_fields, &data_ptr, fields);
                return nb::make_tuple(status,
                                      PyField("", false, std::move(fields), std::dynamic_pointer_cast<const PyMessageDatabase>(decoder.MessageDb())));
            },
            "msg_def_fields"_a, "message_body"_a)
        .def(
            "decode_binary",
            [](DecoderTester& decoder, const std::vector<BaseField::Ptr>& msg_def_fields, const nb::bytes& message_body,
               uint32_t message_length) {
                std::vector<FieldContainer> fields;
                const char* data_ptr = message_body.c_str();
                STATUS status = decoder.TestDecodeBinary(msg_def_fields, reinterpret_cast<const uint8_t**>(&data_ptr),
                                                                                        fields, message_length);
                return nb::make_tuple(status, PyField("", false, std::move(fields), std::dynamic_pointer_cast<const PyMessageDatabase>(decoder.MessageDb())));
            },
            "msg_def_fields"_a, "message_body"_a, "message_length"_a);
}
