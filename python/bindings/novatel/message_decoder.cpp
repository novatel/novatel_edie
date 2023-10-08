#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/variant.h>

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

NB_MAKE_OPAQUE(IntermediateMessage);

struct PyIntermediateMessage
{
    static nb::object values(nb::handle_t<IntermediateMessage> self)
    {
        nb::dict values;
        const auto& message = nb::cast<IntermediateMessage&>(self);
        // FIXME: should store a pointer instead?
        for (const auto& field : message) { values[nb::cast(field.fieldDef->name)] = field.fieldValue; }
        return values;
    }

    static nb::object fields(nb::handle_t<IntermediateMessage> self)
    {
        nb::dict fields;
        const auto& message = nb::cast<IntermediateMessage&>(self);
        for (const auto& field : message) { fields[nb::cast(field.fieldDef->name)] = field.fieldDef; }
        return fields;
    }

    static nb::object get(nb::handle_t<IntermediateMessage> self, std::string field_name)
    {
        auto& message = nb::cast<IntermediateMessage&>(self);
        for (auto& field : message)
        {
            if (field.fieldDef->name == field_name) { return nb::cast(&field.fieldValue); }
        }
        throw nb::key_error(("No '" + field_name + "' field in message").c_str());
    }

    static std::string repr(nb::handle_t<IntermediateMessage> self)
    {
        std::stringstream repr;
        repr << "IntermediateMessage(";
        bool first = true;
        const auto& message = nb::cast<IntermediateMessage&>(self);
        for (const auto& field : message)
        {
            if (!first) { repr << ", "; }
            first = false;
            repr << nb::str("{}={}").format(field.fieldDef->name, field.fieldValue).c_str();
        }
        repr << ")";
        return repr.str();
    }
};

class MessageDecoderWrapper : public novatel::edie::oem::MessageDecoder
{
  public:
    [[nodiscard]] STATUS DecodeBinary_(const std::vector<BaseField*> MsgDefFields_, unsigned char** ppucLogBuf_,
                                       IntermediateMessage& vIntermediateFormat_, uint32_t uiMessageLength_)
    {
        return DecodeBinary(MsgDefFields_, ppucLogBuf_, vIntermediateFormat_, uiMessageLength_);
    }

    [[nodiscard]] STATUS DecodeAscii_(const std::vector<BaseField*> MsgDefFields_, char** ppcLogBuf_, IntermediateMessage& vIntermediateFormat_)
    {
        return DecodeAscii<false>(MsgDefFields_, ppcLogBuf_, vIntermediateFormat_);
    }
};

void init_novatel_message_decoder(nb::module_& m)
{
    nb::bind_vector<IntermediateMessage>(m, "Message")
        .def_prop_ro("values", &PyIntermediateMessage::values)
        .def_prop_ro("fields", &PyIntermediateMessage::fields)
        .def("__getattr__", &PyIntermediateMessage::get, "field_name"_a)
        .def("__getitem__", &PyIntermediateMessage::get, "field_name"_a)
        .def("__repr__", &PyIntermediateMessage::repr)
        .def("__str__", &PyIntermediateMessage::repr);

    nb::class_<FieldContainer>(m, "FieldContainer")
        .def(nb::init<novatel::edie::FieldValueVariant, BaseField*>())
        .def_rw("value", &FieldContainer::fieldValue)
        .def_rw("field_def", &FieldContainer::fieldDef)
        .def("__repr__", [](const FieldContainer& container) {
            return nb::str("FieldContainer(value={}, fieldDef={})").format(container.fieldValue, container.fieldDef);
        });

    nb::class_<oem::MessageDecoder>(m, "MessageDecoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("load_json_db", &oem::MessageDecoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::MessageDecoder::GetLogger)
        .def(
            "decode",
            [](oem::MessageDecoder& decoder, nb::bytes message_body, oem::MetaDataStruct& metadata) {
                IntermediateMessage message;
                STATUS status = decoder.Decode((unsigned char*)message_body.c_str(), message, metadata);
                return nb::make_tuple(status, message);
            },
            "message_body"_a, "metadata"_a)
        // For internal testing purposes only
        .def(
            "_decode_ascii",
            [](oem::MessageDecoder& decoder, const std::vector<BaseField*>& msg_def_fields, nb::bytes message_body) {
                IntermediateMessage message;
                const char* data_ptr = message_body.c_str();
                STATUS status = static_cast<MessageDecoderWrapper*>(&decoder)->DecodeAscii_(msg_def_fields, (char**)&data_ptr, message);
                return nb::make_tuple(status, message);
            },
            "msg_def_fields"_a, "message_body"_a)
        .def(
            "_decode_binary",
            [](oem::MessageDecoder& decoder, const std::vector<BaseField*>& msg_def_fields, nb::bytes message_body, uint32_t message_length) {
                IntermediateMessage message;
                const char* data_ptr = message_body.c_str();
                STATUS status =
                    static_cast<MessageDecoderWrapper*>(&decoder)->DecodeBinary_(msg_def_fields, (unsigned char**)&data_ptr, message, message_length);
                return nb::make_tuple(status, message);
            },
            "msg_def_fields"_a, "message_body"_a, "message_length"_a);
}
