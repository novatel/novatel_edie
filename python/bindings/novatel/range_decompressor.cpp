#include "novatel_edie/decoders/oem/rangecmp/range_decompressor.hpp"

#include <cstring>

#include "bindings_core.hpp"
#include "json_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

// For unit tests
class RangeDecompressorTester : public oem::RangeDecompressor
{
  public:
    void SetBitoffset(uint32_t uiBitOffset_) { uiMyBitOffset = uiBitOffset_; }

    void SetBytesRemaining(uint32_t uiByteCount_) { uiMyBytesRemaining = uiByteCount_; }

    uint32_t GetBytesRemaining() { return uiMyBytesRemaining; }

    uint64_t GetBitfield(uint8_t** ppucBytes_, uint32_t uiBitfieldSize_) { return GetBitfieldFromBuffer(ppucBytes_, uiBitfieldSize_); }
};

void init_novatel_range_decompressor(nb::module_& m)
{
    nb::class_<oem::RangeDecompressor>(m, "RangeDecompressor")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("__init__", [](oem::RangeDecompressor* t) { new (t) oem::RangeDecompressor(JsonDbSingleton::get()); })
        .def("load_json_db", &oem::RangeDecompressor::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::RangeDecompressor::GetLogger)
        .def("reset", &oem::RangeDecompressor::Reset)
        .def(
            "decompress",
            [](oem::RangeDecompressor& self, nb::bytes data, oem::MetaDataStruct& metadata, ENCODE_FORMAT encode_format) -> nb::object {
                if (data.size() > MESSAGE_SIZE_MAX - 1) return nb::make_tuple(STATUS::BUFFER_FULL, nb::none());
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                memcpy(buffer, data.c_str(), data.size());
                buffer[data.size()] = '\0';
                STATUS status = self.Decompress((unsigned char*)buffer, buf_size, metadata, encode_format);
                return nb::make_tuple(status, nb::bytes(buffer, metadata.uiLength));
            },
            "data"_a, "metadata"_a, "encode_format"_a = ENCODE_FORMAT::UNSPECIFIED)
        // For unit tests
        .def_static("_reencode_ChannelTrackingStatusWord", [](uint32_t uiCTS) { return oem::ChannelTrackingStatusStruct(uiCTS).GetAsWord(); })
        .def(
            "_set_bitoffset",
            [](oem::RangeDecompressor& self, uint32_t uiBitOffset_) { static_cast<RangeDecompressorTester*>(&self)->SetBitoffset(uiBitOffset_); },
            "bit_offset"_a)
        .def(
            "_set_bytes_remaining",
            [](oem::RangeDecompressor& self, uint32_t uiByteCount_) {
                static_cast<RangeDecompressorTester*>(&self)->SetBytesRemaining(uiByteCount_);
            },
            "byte_count"_a)
        .def("_get_bytes_remaining", [](oem::RangeDecompressor& self) { return static_cast<RangeDecompressorTester*>(&self)->GetBytesRemaining(); })
        .def(
            "_get_bitfield",
            [](oem::RangeDecompressor& self, nb::bytes data, uint32_t bitfield_size) {
                uint8_t* data_ptr = (uint8_t*)data.c_str();
                uint64_t result = static_cast<RangeDecompressorTester*>(&self)->GetBitfield(&data_ptr, bitfield_size);
                int delta = data_ptr - (uint8_t*)data.c_str();
                return nb::make_tuple(result, data[nb::slice(delta, (int)data.size())]);
            },
            "data"_a, "bitfield_size"_a);
}
