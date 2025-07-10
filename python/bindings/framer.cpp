#include "novatel_edie/decoders/oem/framer.hpp"

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "framer.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

nb::tuple oem::PyFramer::PyGetFrame(uint32_t buffer_size)
{
    std::vector<char> buffer(buffer_size);
    static oem::MetaDataStruct metadata; // maintain metadata until frame is returned
    oem::MetaDataStruct metadata_copy;
    STATUS status = GetFrame(reinterpret_cast<uint8_t*>(buffer.data()), buffer_size, metadata);
    switch (status)
    {
    case STATUS::UNKNOWN: // fall-through
    case STATUS::SUCCESS:
        metadata_copy = oem::MetaDataStruct(metadata);
        metadata = oem::MetaDataStruct();
        return nb::make_tuple(nb::bytes(buffer.data(), metadata_copy.uiLength), metadata_copy);
    default: throw_exception_from_status(status);
    }
}

nb::tuple oem::PyFramer::PyIterGetFrame()
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

void init_novatel_framer(nb::module_& m)
{
    nb::class_<oem::PyFramer>(m, "Framer")
        .def(nb::init(), "Initializes a Framer.")
        .def_prop_rw("frame_json", &oem::PyFramer::GetFrameJson, &oem::PyFramer::SetFrameJson,
                     "Whether to detect, frame, and return messages in JSON format.")
        .def_prop_rw("payload_only", &oem::PyFramer::GetPayloadOnly, &oem::PyFramer::SetPayloadOnly,
                     "Whether to frame and return only the payload of detected messages.")
        .def_prop_rw("report_unknown_bytes", &oem::PyFramer::GetReportUnknownBytes, &oem::PyFramer::SetReportUnknownBytes,
                     "Whether to frame and return undecodable data.")
        .def_prop_ro("available_space", &oem::PyFramer::GetAvailableSpace,
                     "The number of bytes in the Framer's internal buffer available for writing new data.")
        .def("get_frame", &oem::PyFramer::PyGetFrame, "buffer_size"_a = MESSAGE_SIZE_MAX,
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
            "__iter__", [](nb::handle_t<oem::PyFramer> self) { return self; },
            nb::sig("def __iter__(self) -> Iterator[tuple[bytes, MetaData]]"),
            R"doc(
            Marks Framer as Iterable.

            Returns:
                The Framer itself as an Iterator.
            )doc")
        .def("__next__", &oem::PyFramer::PyIterGetFrame, nb::sig("def __next__(self) -> tuple[bytes, MetaData]"),
             R"doc(
            Attempts to get the next frame from the Framer's buffer.

            Returns:
                The framed data and its metadata.

            Raises:
                StopIteration: There is insufficient data in the Framer's
                    buffer to get a complete frame.
            )doc")
        .def(
            "write",
            [](oem::PyFramer& framer, const nb::bytes& data) { return framer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()); },
            R"doc(
            Writes data to the Framer's buffer.

            Use 'available_space' attribute to check how many bytes can be safely written.  

            Args:
                data: The data to write to the buffer.

            Returns:
                The number of bytes written to the Framer's buffer. 
                Can be less than the length of `data` if the buffer is full.
            )doc")
        .def(
            "flush",
            [](oem::PyFramer& framer) {
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                uint32_t flushed = framer.Flush(reinterpret_cast<uint8_t*>(buffer), buf_size);
                return nb::bytes(buffer, flushed);
            },
            R"doc(
            Flushes all bytes from the internal Framer.

            Returns:
                The flushed bytes.
            )doc");
}
