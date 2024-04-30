#include "novatel_edie/decoders/oem/encoder.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

void init_novatel_encoder(nb::module_& m)
{
    nb::class_<oem::Encoder>(m, "Encoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("open", &oem::Encoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::Encoder::GetLogger)
        .def(
            "encode",
            [](oem::Encoder& encoder, oem::IntermediateHeader& header, IntermediateMessage& message,
               novatel::edie::MessageDataStruct& message_data, oem::MetaDataStruct& metadata, ENCODE_FORMAT format) {
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                STATUS status = encoder.Encode((unsigned char**)&buffer, buf_size, header, message, message_data, metadata, format);
                if (status != STATUS::SUCCESS) throw DecoderException(status);
                return nb::bytes(buffer, buf_size);
            },
            "header"_a, "message"_a, "message_data"_a, "metadata"_a, "encode_format"_a);
}
