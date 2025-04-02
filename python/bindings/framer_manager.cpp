#include "novatel_edie/common/framer_manager.hpp"

#include "bindings_core.hpp"
#include "nanobind/nanobind.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_framer_manager(nb::module_& m)
{

    nb::class_<novatel::edie::FramerManager>(m, "FramerManager")
        .def_static("get_instance", &FramerManager::GetInstance, nb::rv_policy::reference)
        .def("register_framer",
             [](FramerManager& self, std::string framerName_, FramerBase* framer_ptr, MetaDataBase* metadata_ptr) {
                 self.RegisterFramer(framerName_, std::move(std::shared_ptr<FramerBase>(framer_ptr)),
                                     std::move(std::shared_ptr<MetaDataBase>(metadata_ptr)));
             })
        .def(
            "metadata", [](novatel::edie::FramerManager& self, int framer_id) { return self.GetMetaData(framer_id); }, "framer_id"_a)
        .def("metadata", [](novatel::edie::FramerManager& self, std::string sFramerName) { return self.GetMetaData(sFramerName); })
        .def(
            "active_metadata", [](FramerManager& self) { return self.GetActiveMetaData(); }, nb::rv_policy::reference)
        .def(
            "reset_metadata", [](FramerManager& self, int framer_id) { self.ResetMetaData(framer_id); }, "framer_id"_a)
        .def("reset_active_framer_id", [](FramerManager& self) { self.ResetActiveFramerId(); })
        .def(
            "set_active_framer_id", [](FramerManager& self, int framer_id) { return self.SetActiveFramerId(framer_id); }, "framer_id"_a)
        .def(
            "set_active_framer_id", [](FramerManager& self, std::string framer_name) { return self.SetActiveFramerId(framer_name); }, "framer_name"_a)
        .def("active_framer_id", [](FramerManager& self) { return self.GetActiveFramerId(); })
        .def(
            "framer_logger", [](FramerManager& self, std::string framer_name) { return self.GetFramerLogger(framer_name).get(); }, "framer_name"_a,
            nb::rv_policy::reference)
        .def("reset_all_framer_states", [](FramerManager& self) { self.ResetAllFramerStates(); })
        .def("reset_all_metadata_states", [](FramerManager& self) { self.ResetAllMetaDataStates(); })
        .def(
            "set_report_unknown_bytes",
            [](FramerManager& self, const bool report_unknown_bytes) { self.SetReportUnknownBytes(report_unknown_bytes); }, "report_unknown_bytes"_a)
        .def_prop_ro("circular_buffer", [](FramerManager& self) { return self.GetCircularBuffer(); })
        .def(
            "framer_element", [](FramerManager& self, int framer_id) { return self.GetFramerElement(framer_id); }, "framer_id"_a,
            nb::rv_policy::reference)
        .def(
            "framer_instance", [](FramerManager& self, std::string framer_name) -> FramerBase* { return self.GetFramerInstance(framer_name).get(); },
            "framer_name"_a, nb::rv_policy::reference)
        .def(
            "write",
            [](FramerManager& self, nb::bytes data) {
                const auto* buffer = reinterpret_cast<const uint8_t*>(data.data());
                size_t size = data.size();
                return self.Write(buffer, static_cast<uint32_t>(size));
            },
            "data"_a)
        .def("sort_framers", [](FramerManager& self) { self.SortFramers(); })
        .def("flush",
             [](FramerManager& self) {
                 char buffer[MESSAGE_SIZE_MAX];
                 uint32_t buf_size = MESSAGE_SIZE_MAX;
                 uint32_t flushed = self.Flush(reinterpret_cast<uint8_t*>(buffer), buf_size);
                 return nb::bytes(buffer, flushed);
             })
        .def_prop_ro(
            "logger", [](FramerManager& self) { return self.GetLogger(); }, nb::rv_policy::reference)
        .def("set_logger_level", [](FramerManager& self, spdlog::level::level_enum eLevel) { self.SetLoggerLevel(eLevel); })
        .def("shutdown_logger", [](FramerManager& self) { self.ShutdownLogger(); })
        .def("get_frame", [](FramerManager& self, uint32_t frameBufferSize) {
            std::vector<char> buffer(frameBufferSize);
            STATUS status = self.GetFrame(reinterpret_cast<uint8_t*>(buffer.data()), frameBufferSize);
            return nb::make_tuple(status, nb::bytes(buffer.data(), buffer.size()));
        });
}
