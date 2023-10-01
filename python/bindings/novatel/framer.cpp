#include "novatel_edie/decoders/oem/framer.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_framer(nb::module_& m)
{
    nb::class_<oem::Framer>(m, "Framer")
        .def(nb::init<>())
        .def("set_frame_json", &oem::Framer::SetFrameJson, "frame_json"_a)
        .def("set_payload_only", &oem::Framer::SetPayloadOnly, "payload_only"_a)
        .def("get_frame", [](oem::Framer& framer) {
            char buffer[MESSAGE_SIZE_MAX];
            uint32_t buf_size = MESSAGE_SIZE_MAX;
            oem::MetaDataStruct metadata;
            STATUS status = framer.GetFrame((unsigned char*)buffer, buf_size, metadata);
            return nb::make_tuple(nb::bytes(buffer, buf_size), status, metadata);
        });
}
