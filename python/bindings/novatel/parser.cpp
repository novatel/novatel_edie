#include "novatel_edie/decoders/oem/parser.hpp"

#include "bindings_core.hpp"
#include "json_db_singleton.hpp"
#include "py_message_data.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_parser(nb::module_& m)
{
    nb::class_<oem::Parser>(m, "Parser")
        .def(nb::init<std::u32string>(), "json_db_path"_a)
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("__init__", [](oem::Parser* t) { new (t) oem::Parser(JsonDbSingleton::get()); })
        .def("load_json_db", &oem::Parser::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::Parser::GetLogger)
        .def("enable_framer_decoder_logging", &oem::Parser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug, "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::Parser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::Parser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::Parser::GetDecompressRangeCmp, &oem::Parser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::Parser::GetReturnUnknownBytes, &oem::Parser::SetReturnUnknownBytes)
        .def_prop_rw("encode_format", &oem::Parser::GetEncodeFormat, &oem::Parser::SetEncodeFormat)
        .def_prop_rw("filter", &oem::Parser::GetFilter, &oem::Parser::SetFilter, nb::rv_policy::reference_internal)
        .def("write", [](oem::Parser& self, nb::bytes data) { return self.Write((unsigned char*)data.c_str(), data.size()); })
        .def("read",
             [](oem::Parser& self) {
                 novatel::edie::MessageDataStruct message_data;
                 oem::MetaDataStruct meta_data;
                 STATUS status = self.Read(message_data, meta_data);
                 return std::make_tuple(status, oem::PyMessageData(std::move(message_data)), std::move(meta_data));
             })
        .def(
            "flush",
            [](oem::Parser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) return nb::int_(self.Flush());
                char buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush((unsigned char*)buffer, oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false)
        .def_prop_ro("internal_buffer",
                     [](oem::Parser& self) { return nb::bytes((char*)self.GetInternalBuffer(), oem::Parser::uiParserInternalBufferSize); });
}
