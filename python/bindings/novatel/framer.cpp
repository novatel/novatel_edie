#include "novatel_edie/decoders/oem/framer.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_framer(nb::module_& m)
{
    nb::class_<oem::Framer>(m, "Framer")
        .def(nb::init<>())
        .def_prop_ro("logger", [](oem::Framer& self) { return self.GetLogger(); })
        .def("set_frame_json", &oem::Framer::SetFrameJson, "frame_json"_a)
        .def("set_payload_only", &oem::Framer::SetPayloadOnly, "payload_only"_a)
        .def(
            "set_report_unknown_bytes", [](oem::Framer& self, bool report_unknown_bytes) { self.SetReportUnknownBytes(report_unknown_bytes); },
            "report_unknown_bytes"_a)
        .def("get_bytes_available_in_buffer", &oem::Framer::GetBytesAvailableInBuffer)
        .def("get_frame",
             [](oem::Framer& framer) {
                 char buffer[MESSAGE_SIZE_MAX];
                 uint32_t buf_size = MESSAGE_SIZE_MAX;
                 oem::MetaDataStruct metadata;
                 STATUS status = framer.GetFrame((unsigned char*)buffer, buf_size, metadata);
                 return nb::make_tuple(status, nb::bytes(buffer, metadata.uiLength), metadata);
             })
        .def("write", [](oem::Framer& framer, nb::bytes data) { return framer.Write((unsigned char*)data.c_str(), data.size()); })
        .def("flush", [](oem::Framer& framer) {
            char buffer[MESSAGE_SIZE_MAX];
            uint32_t buf_size = MESSAGE_SIZE_MAX;
            uint32_t flushed = framer.Flush((unsigned char*)buffer, buf_size);
            return nb::bytes(buffer, flushed);
        });
}
