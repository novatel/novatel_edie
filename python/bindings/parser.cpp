#include "novatel_edie/decoders/oem/parser.hpp"

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_parser(nb::module_& m)
{
    nb::class_<oem::Parser>(m, "Parser")
        .def("__init__", [](oem::Parser* t) { new (t) oem::Parser(MessageDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def(nb::init<const std::filesystem::path&>(), "json_db_path"_a)
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "message_db"_a)
        .def("load_json_db", &oem::Parser::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::Parser::GetLogger)
        .def("enable_framer_decoder_logging", &oem::Parser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug, "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::Parser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::Parser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::Parser::GetDecompressRangeCmp, &oem::Parser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::Parser::GetReturnUnknownBytes, &oem::Parser::SetReturnUnknownBytes)
        .def_prop_rw("encode_format", &oem::Parser::GetEncodeFormat, &oem::Parser::SetEncodeFormat)
        .def_prop_rw("filter", &oem::Parser::GetFilter, &oem::Parser::SetFilter)
        .def("metadata", &oem::Parser::GetMetaData, nb::rv_policy::reference)
        .def("write",
             [](oem::Parser& self, const nb::bytes& data) { return self.Write(reinterpret_cast<const unsigned char*>(data.c_str()), data.size()); })
        .def("read",
             [](oem::Parser& self) {
                 MessageDataStruct message_data;
                 STATUS status = self.Read(message_data);
                 return std::make_tuple(status, oem::PyMessageData(message_data));
             })
        .def(
            "read",
            [](oem::Parser& self, bool decode_incomplete) {
                MessageDataStruct message_data;
                STATUS status = self.Read(message_data, decode_incomplete);
                return std::make_tuple(status, oem::PyMessageData(message_data));
            },
            "decode_incomplete_abbreviated"_a = false)
        .def("__iter__", [](oem::Parser& self) { return &self; })
        .def("__next__",
             [](oem::Parser& self) {
                 MessageDataStruct message_data;
                 STATUS status = self.Read(message_data);
                 switch (status)
                 {
                 case STATUS::BUFFER_EMPTY: [[fallthrough]];
                 case STATUS::INCOMPLETE: [[fallthrough]];
                 case STATUS::INCOMPLETE_MORE_DATA: throw nb::stop_iteration();
                 default: break;
                 }
                 return std::make_tuple(status, oem::PyMessageData(message_data));
             })
        .def(
            "flush",
            [](oem::Parser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                uint8_t buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush(buffer, oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false)
        .def_prop_ro("internal_buffer",
                     [](const oem::Parser& self) { return nb::bytes(self.GetInternalBuffer(), oem::Parser::uiParserInternalBufferSize); });
}
