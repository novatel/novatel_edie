#include "novatel_edie/decoders/oem/framer.hpp"

#include "bindings_core.hpp"
#include "novatel_edie/decoders/common/framer.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_framer(nb::module_& m)
{
    nb::class_<novatel::edie::FramerBase>(m, "FramerBase");

    nb::class_<oem::Framer, novatel::edie::FramerBase>(m, "Framer")
        .def(nb::init(), nb::rv_policy::move)
        .def_prop_ro("logger", [](const oem::Framer& self) { return self.GetLogger(); })
        .def(
            "set_frame_json", [](oem::Framer& self, bool frame_json) { self.SetFrameJson(frame_json); }, "frame_json"_a)
        .def(
            "set_payload_only", [](oem::Framer& self, bool payload_only) { self.SetPayloadOnly(payload_only); }, "payload_only"_a)
        .def_prop_ro("bytes_available_in_buffer", [](const oem::Framer& self) { return self.GetBytesAvailableInBuffer(); })
        .def("find_next_sync_byte", [](oem::Framer& self, uint32_t buffer_size) { return self.FindNextSyncByte(buffer_size); });

    m.def("create_framer", []() -> novatel::edie::FramerBase* { return new oem::Framer(); });
}
