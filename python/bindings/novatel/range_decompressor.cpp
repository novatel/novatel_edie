#include "novatel_edie/decoders/oem/rangecmp/range_decompressor.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_range_decompressor(nb::module_& m)
{
    nb::class_<oem::RangeDecompressor>(m, "RangeDecompressor")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("load_json_db", &oem::RangeDecompressor::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::RangeDecompressor::GetLogger)
        .def("reset", &oem::RangeDecompressor::Reset)
        .def(
            "decompress",
            [](oem::RangeDecompressor& self, nb::bytes data, oem::MetaDataStruct& metadata, ENCODE_FORMAT encode_format) {
                STATUS status = self.Decompress((unsigned char*)data.c_str(), data.size(), metadata, encode_format);
                return status;
            },
            "data"_a, "metadata"_a, "encode_format"_a = ENCODE_FORMAT::UNSPECIFIED);
}
