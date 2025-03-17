#include "novatel_edie/decoders/oem/framer.hpp"

#include "bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_framer(nb::module_& m)
{
    nb::class_<oem::Framer>(m, "Framer")
        .def(nb::init())
        .def_prop_ro("logger", [](const oem::Framer& self) { return self.GetLogger(); })
        .def(
            "set_frame_json", [](oem::Framer& self, bool frame_json) { self.SetFrameJson(frame_json); }, "frame_json"_a)
        .def(
            "set_payload_only", [](oem::Framer& self, bool payload_only) { self.SetPayloadOnly(payload_only); }, "payload_only"_a)
        .def_prop_ro("bytes_available_in_buffer", [](const oem::Framer& self) { return self.GetBytesAvailableInBuffer(); })
        .def("find_next_sync_byte", [](oem::Framer& self, uint32_t buffer_size) { return self.FindNextSyncByte(buffer_size); })
        .def("get_frame", [](oem::Framer& self, MetaDataBase* metadatabase_ptr, uint32_t buffer_size) {
            std::vector<char> buffer(buffer_size);
            STATUS status = self.GetFrame(reinterpret_cast<uint8_t*>(buffer.data()), buffer_size, *metadatabase_ptr);
            return nb::make_tuple(status, nb::bytes(buffer.data(), metadatabase_ptr->uiLength));
        });
}
