#include "bindings_core.h"
#include "novatel_edie/decoders/oem/file_parser.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_file_parser(nb::module_& m)
{
    nb::class_<oem::FileParser>(m, "FileParser")
        .def(nb::init<std::u32string>(), "json_db_path"_a)
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("load_json_db", &oem::FileParser::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::FileParser::GetLogger)
        .def("enable_framer_decoder_logging", &oem::FileParser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug,
             "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::FileParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::FileParser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_ro("percent_read", &oem::FileParser::GetPercentRead)
        .def_prop_rw("decompress_range_cmp", &oem::FileParser::GetDecompressRangeCmp, &oem::FileParser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::FileParser::GetReturnUnknownBytes, &oem::FileParser::SetReturnUnknownBytes)
        .def_prop_rw("encode_format", &oem::FileParser::GetEncodeFormat, &oem::FileParser::SetEncodeFormat)
        .def_prop_rw("filter", &oem::FileParser::GetFilter, &oem::FileParser::SetFilter)
        .def("set_stream", &oem::FileParser::SetStream, "input_stream"_a)
        .def("read",
             [](oem::FileParser& self) {
                 novatel::edie::MessageDataStruct stMessageData;
                 oem::MetaDataStruct stMetaData;
                 STATUS status = self.Read(stMessageData, stMetaData);
                 return std::make_tuple(status, stMessageData, stMetaData);
             })
        .def("reset", &oem::FileParser::Reset)
        .def(
            "flush",
            [](oem::FileParser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) return nb::int_(self.Flush());
                char buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush((unsigned char*)buffer, oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false)
        .def_prop_ro("internal_buffer",
                     [](oem::FileParser& self) { return nb::bytes((char*)self.GetInternalBuffer(), oem::Parser::uiParserInternalBufferSize); });
}
