#include "novatel_edie/decoders/oem/encoder.hpp"

#include "bindings_core.hpp"
#include "json_db_singleton.hpp"
#include "py_intermediate_message.hpp"
#include "py_message_data.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_encoder(nb::module_& m)
{
    define_pymessagedata(m);

    nb::class_<oem::Encoder>(m, "Encoder")
        .def(nb::init<JsonReader::Ptr&>(), "json_db"_a)
        .def("__init__", [](oem::Encoder* t) { new (t) oem::Encoder(JsonDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def("open", &oem::Encoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", [](const oem::Encoder& encoder) { return encoder.GetLogger(); })
        .def(
            "encode",
            [](oem::Encoder& encoder, const oem::IntermediateHeader& header, const PyIntermediateMessage& py_message,
               const oem::MetaDataStruct& metadata, ENCODE_FORMAT format) {
                MessageDataStruct message_data;
                if (format == ENCODE_FORMAT::JSON)
                {
                    // Allocate more space for JSON messages.
                    // A TRACKSTAT message can use about 47k bytes when encoded as JSON.
                    // FIXME: this is still not safe and there is no effective buffer overflow checking implemented in Encoder.
                    uint8_t buffer[MESSAGE_SIZE_MAX * 3];
                    auto* buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
                    uint32_t buf_size = MESSAGE_SIZE_MAX * 3;
                    STATUS status = encoder.Encode(&buf_ptr, buf_size, header, py_message.message, message_data, metadata, format);
                    return nb::make_tuple(status, oem::PyMessageData(message_data));
                }
                else
                {
                    uint8_t buffer[MESSAGE_SIZE_MAX];
                    auto buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
                    uint32_t buf_size = MESSAGE_SIZE_MAX;
                    STATUS status = encoder.Encode(&buf_ptr, buf_size, header, py_message.message, message_data, metadata, format);
                    return nb::make_tuple(status, oem::PyMessageData(message_data));
                }
            },
            "header"_a, "message"_a, "metadata"_a, "encode_format"_a);
}