#include "novatel_edie/common/framer_manager.hpp"

#include "bindings_core.hpp"
#include "nanobind/nanobind.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_framer_manager(nb::module_& m)
{
    nb::class_<FramerManager>(m, "FramerManager")
        .def_static("get_instance", &FramerManager::GetInstance, nb::rv_policy::reference)
        //.def("register_framer",
        //     [](FramerManager& self, std::string framerName_, std::unique_ptr<FramerBase> framerbase_ptr,
        //        std::unique_ptr<MetaDataBase> metadatabase_ptr) {
        //         self.RegisterFramer(framerName_, std::move(framerbase_ptr), std::move(metadatabase_ptr));
        //     })
        .def("metadata", [](novatel::edie::FramerManager& self, int iFramerId) { return self.GetMetaData(iFramerId); })
        .def("metadata", [](novatel::edie::FramerManager& self, std::string sFramerName) { return self.GetMetaData(sFramerName); })
        .def("active_metadata", [](FramerManager& self) { return self.GetActiveMetaData(); })
        .def("reset_metadata", [](FramerManager& self, int iFramerId) { self.ResetMetaData(iFramerId); })
        .def("reset_active_Framer_id", [](FramerManager& self) { self.ResetActiveFramerId(); })
        .def("set_active_Framer_id", [](FramerManager& self, int iFramerId) { return self.SetActiveFramerId(iFramerId); })
        .def("set_active_Framer_id", [](FramerManager& self, std::string sFramerName) { return self.SetActiveFramerId(sFramerName); })
        .def(
            "active_Framer_id", [](FramerManager& self) { return self.ActiveFramerId(); })
        .def("reset_all_framer_states", [](FramerManager& self) { self.ResetAllFramerStates(); })
        .def("reset_all_metadata_states", [](FramerManager& self) { self.ResetAllMetaDataStates(); })
        .def(
            "set_report_unknown_bytes", [](FramerManager& self, const bool report_unknown_bytes) { self.SetReportUnknownBytes(report_unknown_bytes); },
            "report_unknown_bytes"_a)
        .def_prop_ro("circular_buffer", [](FramerManager& self) { return self.GetCircularBuffer(); })
        .def_prop_ro("framer_element", [](FramerManager& self, int framerId) { return self.GetFramerElement(framerId); })
        .def_prop_ro(
            "framer_instance", [](FramerManager& self, std::string sFramerName) -> FramerBase* { return self.GetFramerInstance(sFramerName).get(); })
        .def("write",
             [](FramerManager& self, const unsigned char* pucDataBuffer, uint32_t uiDataBytes) { return self.Write(pucDataBuffer, uiDataBytes); })
        .def("sort_framers", [](FramerManager& self) { self.SortFramers(); })
        .def("flush", [](FramerManager& self, nb::bytearray& data) { self.Flush(reinterpret_cast<uint8_t*>(data.data()), data.size()); })

        .def_prop_ro("logger", [](FramerManager& self) { return self.GetLogger(); })
        .def(
            "set_logger_level", [](FramerManager& self, spdlog::level::level_enum eLevel) { self.SetLoggerLevel(eLevel); })
        .def("shutdown_logger", [](FramerManager& self) { self.ShutdownLogger(); })
        .def("get_frame",
             [](FramerManager& self, unsigned char* frameBuffer, uint32_t frameBufferSize) { return self.GetFrame(frameBuffer, frameBufferSize); });
}
