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
        .def(nb::init())
        //.def_prop_ro("logger", [](const oem::PyFramer& self) { return self.GetLogger(); })
        .def(
            "set_frame_json", [](oem::PyFramer& self, bool frame_json) { self.SetFrameJson(frame_json); }, "frame_json"_a)
        .def(
            "set_payload_only", [](oem::PyFramer& self, bool payload_only) { self.SetPayloadOnly(payload_only); }, "payload_only"_a)
        .def(
            "set_report_unknown_bytes", [](oem::PyFramer& self, bool report_unknown_bytes) { self.SetReportUnknownBytes(report_unknown_bytes); },
            "report_unknown_bytes"_a)
        .def_prop_ro("bytes_available_in_buffer", [](const oem::PyFramer& framer) { return framer.GetBytesAvailableInBuffer(); })
        .def("get_frame", &oem::PyFramer::PyGetFrame, "buffer_size"_a = MESSAGE_SIZE_MAX)
        .def("__iter__", [](nb::handle_t<oem::Framer> self) { return self; })
        .def("__next__", &oem::PyFramer::PyIterGetFrame)
        .def("write",
             [](oem::PyFramer& framer, const nb::bytes& data) { return framer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()); })
        .def("flush", [](oem::PyFramer& framer) {
            char buffer[MESSAGE_SIZE_MAX];
            uint32_t buf_size = MESSAGE_SIZE_MAX;
            uint32_t flushed = framer.Flush(reinterpret_cast<uint8_t*>(buffer), buf_size);
            return nb::bytes(buffer, flushed);
        });
}
