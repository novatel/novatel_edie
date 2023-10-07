#include "novatel_edie/decoders/oem/encoder.hpp"

#include "bindings_core.hpp"
#include "py_message_data.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

void init_novatel_encoder(nb::module_& m)
{
    define_pymessagedata(m);

    nb::class_<oem::Encoder>(m, "Encoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("open", &oem::Encoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::Encoder::GetLogger)
        .def(
            "encode",
            [](oem::Encoder& encoder, oem::IntermediateHeader& header, IntermediateMessage& message, oem::MetaDataStruct& metadata,
               ENCODE_FORMAT format) {
                unsigned char buffer[MESSAGE_SIZE_MAX];
                unsigned char* buf_ptr = (unsigned char*)&buffer;
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                novatel::edie::MessageDataStruct message_data;
                STATUS status = encoder.Encode(&buf_ptr, buf_size, header, message, message_data, metadata, format);
                return nb::make_tuple(status, oem::PyMessageData(message_data));
            },
            "header"_a, "message"_a, "metadata"_a, "encode_format"_a);
}
