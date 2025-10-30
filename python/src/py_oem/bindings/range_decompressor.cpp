#include "novatel_edie/decoders/oem/rangecmp/range_decompressor.hpp"

#include <cstring>

#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_oem/bindings.hpp"
#include "py_common/message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

void py_oem::init_novatel_range_decompressor(nb::module_& m)
{
    nb::class_<oem::RangeDecompressor>(m, "RangeDecompressor")
        .def(
            "__init__",
            [](oem::RangeDecompressor* t, py_common::PyMessageDatabaseCore::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); };
                new (t) oem::RangeDecompressor(message_db);
                t->GetLogger()->warn(
                    "The RangeDecompressor interface is currently unstable! It may undergo breaking changes between minor version increments.");
            },
            nb::arg("message_db") = nb::none()) // NOLINT(*.NewDeleteLeaks)
        .def("reset", &oem::RangeDecompressor::Reset)
        .def(
            "decompress",
            [](oem::RangeDecompressor& self, const nb::bytes& data, oem::MetaDataStruct& metadata, ENCODE_FORMAT encode_format) -> nb::object {
                if (data.size() > MESSAGE_SIZE_MAX - 1) { return nb::make_tuple(STATUS::BUFFER_FULL, nb::none()); }
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                memcpy(buffer, data.c_str(), data.size());
                buffer[data.size()] = '\0';
                STATUS status = self.Decompress(reinterpret_cast<uint8_t*>(buffer), buf_size, metadata, encode_format);
                throw_exception_from_status(status);
                return nb::bytes(buffer, metadata.uiLength);
            },
            "data"_a, "metadata"_a, "encode_format"_a = ENCODE_FORMAT::UNSPECIFIED)
        // For unit tests
        .def_static("_reencode_ChannelTrackingStatusWord", [](uint32_t uiCTS) { return oem::ChannelTrackingStatus(uiCTS).GetAsWord(); });
}
