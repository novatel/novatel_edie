#include "novatel_edie/decoders/oem/header_decoder.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_header_decoder(nb::module_& m)
{
    nb::class_<oem::HeaderDecoder>(m, "HeaderDecoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("load_json_db", &oem::HeaderDecoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::HeaderDecoder::GetLogger)
        .def(
            "decode",
            [](oem::HeaderDecoder& decoder, nb::bytes header, oem::MetaDataStruct& metadata) {
                oem::IntermediateHeader intermediate_header;
                STATUS status = decoder.Decode((unsigned char*)header.c_str(), intermediate_header, metadata);
                if (status != STATUS::SUCCESS) throw DecoderException(status);
                return nb::make_tuple(intermediate_header, metadata);
            },
            "header"_a, "metadata"_a);
}
