#include "novatel_edie/decoders/oem/header_decoder.hpp"

#include "bindings_core.hpp"
#include "json_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_header_decoder(nb::module_& m)
{
    nb::class_<oem::HeaderDecoder>(m, "HeaderDecoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("__init__", [](oem::HeaderDecoder* t) { new (t) oem::HeaderDecoder(JsonDbSingleton::get()); })
        .def("load_json_db", &oem::HeaderDecoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::HeaderDecoder::GetLogger)
        .def(
            "decode",
            [](oem::HeaderDecoder& decoder, nb::bytes raw_header, oem::MetaDataStruct& metadata) {
                oem::IntermediateHeader header;
                STATUS status = decoder.Decode((unsigned char*)raw_header.c_str(), header, metadata);
                return nb::make_tuple(status, header);
            },
            "header"_a, "metadata"_a);
}
