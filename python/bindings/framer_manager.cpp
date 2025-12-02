#include "novatel_edie/decoders/common/framer_manager.hpp"

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "framer_manager.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

nb::tuple PyFramerManager::PyGetFrame(uint32_t buffer_size)
{
    std::vector<char> buffer(buffer_size);
    static MetaDataBase* metadata; // maintain metadata until frame is returned
    MetaDataBase metadata_copy;
    STATUS status = GetFrame(reinterpret_cast<uint8_t*>(buffer.data()), buffer_size, metadata);
    switch (status)
    {
    case STATUS::UNKNOWN: // fall-through
    case STATUS::SUCCESS:
        metadata_copy = MetaDataBase(*metadata);
        metadata = new MetaDataBase();
        return nb::make_tuple(nb::bytes(buffer.data(), metadata_copy.uiLength), metadata_copy);
    default: throw_exception_from_status(status);
    }
}

nb::tuple PyFramerManager::PyIterGetFrame()
{
    try
    {
        return PyGetFrame(MESSAGE_SIZE_MAX);
    }
    catch (IncompleteException)
    {
    }
    catch (IncompleteMoreDataException)
    {
    }
    catch (BufferEmptyException)
    {
    }
    throw nb::stop_iteration("No more frames detected in buffer");
}

void init_common_framer_manager(nb::module_& m)
{
    nb::class_<PyFramerManager>(m, "FramerManager")
        // Constructor that accepts a list of strings (framers)
        .def(nb::init<const std::vector<std::string>>(), "selected_framers"_a, "Constructs a FramerManager with a list of selected framers by name")
        .def("register_framer",
             [](FramerManager& self, const std::string& framer_name, nb::callable py_framer_factory, nb::callable py_metadata_constructor) {
                 auto framer_factory = [=](std::shared_ptr<UCharFixedRingBuffer> rb) -> std::unique_ptr<FramerBase> {
                     nb::object factory = py_framer_factory(rb);
                     return std::move(nb::cast<std::unique_ptr<FramerBase>&>(factory));
                 };

                 auto metadata_constructor = [=]() -> std::unique_ptr<MetaDataBase> {
                     nb::object constructor = py_metadata_constructor();
                     return std::move(nb::cast<std::unique_ptr<MetaDataBase>&>(constructor));
                 };
                 self.GetLogger()->debug("Registering framer: {}", framer_name);
                 self.RegisterFramer(framer_name, framer_factory, metadata_constructor);
             })
        .def("metadata", [](novatel::edie::FramerManager& self, std::string sFramerName) { return self.GetMetaData(sFramerName); })
        .def("list_available_framers", [](FramerManager& self) { self.ListAvailableFramers(); })
        .def("reset_all_framer_states", [](FramerManager& self) { self.ResetAllFramerStates(); })
        .def(
            "set_report_unknown_bytes",
            [](FramerManager& self, const bool report_unknown_bytes) { self.SetReportUnknownBytes(report_unknown_bytes); }, "report_unknown_bytes"_a)
        .def_prop_rw("report_unknown_bytes", &PyFramerManager::GetReportUnknownBytes, &PyFramerManager::SetReportUnknownBytes,
                     "Whether to frame and return undecodable data.")
        .def_prop_ro("available_space", &PyFramerManager::GetAvailableSpace,
                     "The number of bytes in the Framer Manager's internal buffer available for writing new data.")
        .def_prop_ro("circular_buffer", [](FramerManager& self) { return self.GetFixedRingBuffer(); })
        .def(
            "framer_element", [](FramerManager& self, std::string framer_name) { return self.GetFramerElement(framer_name); }, "framer_id"_a,
            nb::rv_policy::reference)
        .def("active_framer_name", [](FramerManager& self) { return self.GetActiveFramerName(); })
        .def(
            "write",
            [](PyFramerManager& framer_manager, const nb::bytes& data) {
                return framer_manager.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            },
            R"doc(
            Writes data to the Framer's buffer.

            Args:
                data: The data to write to the buffer.
            )doc")
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
        .def("get_frame", &PyFramerManager::PyGetFrame, "buffer_size"_a = MESSAGE_SIZE_MAX,
             nb::sig("def get_frame(buffer_size = MAX_MESSAGE_LENGTH) -> tuple[bytes, MetaData]"),
             R"doc(
            Attempts to get a frame from the Framer's buffer.

            Args:
                buffer_size: The maximum number of bytes to use for a framed message.

            Returns:
                The framed data and metadata.

            Raises:
                BufferEmptyException: There are no more bytes in the internal buffer.
                BufferFullException: The framed message does not fit in the provided
                    buffer size.
                IncompleteException: The framer found the start of a message, but 
                    there are no more bytes in the internal buffer.
            )doc")
        .def(
            "__iter__", [](nb::handle_t<PyFramerManager> self) { return self; },
            nb::sig("def __iter__(self) -> Iterator[tuple[bytes, MetaData]]"),
            R"doc(
            Marks Framer Manager as Iterable.

            Returns:
                The Framer Manager itself as an Iterator.
            )doc")
        .def("__next__", &PyFramerManager::PyIterGetFrame, nb::sig("def __next__(self) -> tuple[bytes, MetaData]"),
             R"doc(
            Attempts to get the next frame from the Framer Manager's buffer.

            Returns:
                The framed data and its metadata.

            Raises:
                StopIteration: There is insufficient data in the Framer Manager's buffer to get a complete frame.
            )doc");
}
