#include "novatel_edie/decoders/common/framer_manager.hpp"

#include "bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_framer_manager(nb::module_& m)
{
    nb::class_<FramerManager>(m, "FramerManager")
        .def(nb::init())
        .def("register_framer", [](FramerManager& self, std::string framerName_, FramerBase* framerbase_ptr,
                                   MetaDataBase* metadatabase_ptr) { self.RegisterFramer(framerName_, framerbase_ptr, metadatabase_ptr) })

        .def_prop("metadata", [](FramerManager& self, int iFramerId) { return self.GetMetadata(iFramerId); })
        .def_prop("active_metadata", [](FramerManager& self, int iFramerId) { return self.GetActiveMetadata(); })
        .def("reset_active_Framer_id", [](FramerManager& self) { self.ResetActiveFramerId(); })
        .def("set_active_Framer_id", [](FramerManager& self, int iFramerId) { return self.SetActiveFramerId(iFramerId); })
        .def("set_active_Framer_id", [](FramerManager& self, std::string sFramerId) { return self.SetActiveFramerId(sFramerId); })
        .def(
            "active_Framer_id", [](FramerManager& self) { return self.ActiveFramerId(sFramerId); }, "active_framer_id"_a)
        .def("reset_all_framer_states", [](FramerManager& self) { self.ResetAllFramerStates(); })
        .def("reset_all_metadata_states", [](FramerManager& self) { self.ResetAllMetaDataStates(); })
        .def(
            "set_report_unknown_bytes", [](FramerManager& self, bool report_unknown_bytes) { self.SetReportUnknownBytes(report_unknown_bytes); },
            "report_unknown_bytes"_a)
        .def_static("get_instance", &novatel::edie::FramerManager::GetInstance, nb::return_value_policy::reference)
        .def_prop("circular_buffer", [](const novatel::edie::oem::Framer& self) { return self.GetCircularBuffer(); })
        .def_prop("framer_element", [](FramerManager& self, int framerId) { return self.GetFramerElement(framerId); })
        .def_prop("framer_instance", [](FramerManager& self, std::string framerName) { return self.GetFramerInstance(framerName); })
        .def("write",
             [](FramerManager& self, const unsigned char* pucDataBuffer, uint32_t uiDataBytes) { return self.Write(pucDataBuffer, uiDataBytes); })
        .def("sort_framers", [](FramerManager& self) { self.SortFramers(); })
        .def("flush", [](FramerManager& self, const nb::bytes& buffer,
                         uint32_t uiBufferSize) { return self.Flush(reinterpret_cast<const uint8_t*>(buffer.c_str()), uiBufferSize); })
        .def_prop_ro("logger", [](const oem::Framer& self) { return self.GetLogger(); })
        .def("shutdown_logger", [](FramerManager& self) { self.LoggerShutdown(); })
        .def("get_frame",
             [](FramerManager& self, unsigned char* frameBuffer uint32_t frameBufferSize) { return self.GetFrame(frameBuffer, frameBufferSize); });
}
