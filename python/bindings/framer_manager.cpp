#include "novatel_edie/decoders/common/framer_manager.hpp"
#include "novatel_edie/decoders/oem/framer.hpp"

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
    thread_local MetaDataBase* metadata; // maintain metadata until frame is returned
    STATUS status = GetFrame(reinterpret_cast<uint8_t*>(buffer.data()), buffer_size, metadata);
    switch (status)
    {
    case STATUS::UNKNOWN: // fall-through
    case STATUS::SUCCESS:
    {
        nb::bytes frame_bytes(buffer.data(), metadata->uiLength);

        // Check which framer produced this frame
        if (GetActiveFramerId() == FramerManager::GetFramerId("OEM")) {
            auto* oem_meta = reinterpret_cast<oem::MetaDataStruct*>(metadata);
            if (oem_meta) {
                return nb::make_tuple(frame_bytes, *oem_meta);
            }
        }
        
        return nb::make_tuple(frame_bytes, *metadata);
    }
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
        // Constructor that accepts a list of strings (framer names)
        .def(nb::init<const std::vector<std::string>>(), "selected_framers"_a, "Constructs a FramerManager with a list of selected framers by name")
        .def("reset_all_framer_states", [](PyFramerManager& self) { self.ResetAllFramerStates(); })
        .def_prop_rw("report_unknown_bytes",
                     &PyFramerManager::GetReportUnknownBytes,
                     &PyFramerManager::SetReportUnknownBytes,
                     "Whether to frame and return undecodable data.")
        .def_prop_ro("available_space",
                     &PyFramerManager::GetAvailableSpace,
                     "The number of bytes in the Framer Manager's internal buffer available for writing new data.")
        .def_prop_ro("active_framer_name", 
                     &PyFramerManager::GetActiveFramerName,
                     "The name of the framer that successfully framed the last message")
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
             [](PyFramerManager& self) {
                 char buffer[MESSAGE_SIZE_MAX];
                 uint32_t buf_size = MESSAGE_SIZE_MAX;
                 uint32_t flushed = self.Flush(reinterpret_cast<uint8_t*>(buffer), buf_size);
                 return nb::bytes(buffer, flushed);
             })
        .def("get_frame", &PyFramerManager::PyGetFrame, "buffer_size"_a = MESSAGE_SIZE_MAX,
             nb::sig("def get_frame(buffer_size = MAX_MESSAGE_LENGTH) -> tuple[bytes, MetaData]"),
             R"doc(
            Attempts to get a frame from the Framer Manager's buffer.

            Args:
                buffer_size: The maximum number of bytes to use for a framed message.

            Returns:
                The framed data and metadata.

            Raises:
                BufferEmptyException: There are no more bytes in the internal buffer.
                BufferFullException: The framed message does not fit in the provided
                    buffer size.
                IncompleteException: The framer manager found the start of a message, but 
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
