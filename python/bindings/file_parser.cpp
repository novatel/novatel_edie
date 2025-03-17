#include "novatel_edie/decoders/oem/file_parser.hpp"

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"
#include "pystream.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_file_parser(nb::module_& m)
{
    nb::class_<oem::FileParser>(m, "FileParser")
        .def("__init__", [](oem::FileParser* t) { new (t) oem::FileParser(MessageDbSingleton::get()); }) // NOLINT(*-cplusplus.NewDeleteLeaks)
        .def(nb::init<const std::filesystem::path&>(), "json_db_path"_a)
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "message_db"_a)
        .def("load_json_db", &oem::FileParser::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::FileParser::GetLogger)
        .def("enable_framer_decoder_logging", &oem::FileParser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug,
             "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::FileParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::FileParser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::FileParser::GetDecompressRangeCmp, &oem::FileParser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::FileParser::GetReturnUnknownBytes, &oem::FileParser::SetReturnUnknownBytes)
        .def_prop_rw("encode_format", &oem::FileParser::GetEncodeFormat, &oem::FileParser::SetEncodeFormat)
        .def_prop_rw("filter", &oem::FileParser::GetFilter, &oem::FileParser::SetFilter)
        .def(
            "set_stream", [](oem::FileParser& self, nb::object stream) { return self.SetStream(std::make_shared<pystream::istream>(stream, 0)); },
            "input_stream"_a)
        .def("read",
             [](oem::FileParser& self) {
                 MessageDataStruct message_data;
                 STATUS status = self.Read(message_data);
                 FramerManager& framerManager = FramerManager::GetInstance();
                 auto* metaData_ptr = framerManager.GetMetaData("NOVATEL");
                 return std::make_tuple(status, oem::PyMessageData(message_data), *metaData_ptr);
             })
        .def("__iter__", [](oem::FileParser& self) { return &self; })
        .def("__next__",
             [](oem::FileParser& self) {
                 MessageDataStruct message_data;
                 STATUS status = self.Read(message_data);
                 FramerManager& framerManager = FramerManager::GetInstance();
                 auto& metaData = framerManager.GetMetaData("NOVATEL");
                 if (status == STATUS::STREAM_EMPTY) { throw nb::stop_iteration(); }
                 return std::make_tuple(status, oem::PyMessageData(message_data), metaData);
             })
        .def("reset", &oem::FileParser::Reset)
        .def(
            "flush",
            [](oem::FileParser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                char buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush(reinterpret_cast<uint8_t*>(buffer), oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false)
        .def_prop_ro("internal_buffer",
                     [](const oem::FileParser& self) { return nb::bytes(self.GetInternalBuffer(), oem::Parser::uiParserInternalBufferSize); });
}
