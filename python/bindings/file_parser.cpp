#include "novatel_edie/decoders/oem/file_parser.hpp"

#include "bindings_core.hpp"
#include "file_parser.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"
#include "pystream.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

nb::object oem::PyFileParser::PyRead()
{
    MetaDataStruct metadata;
    MessageDataStruct message_data;
    PyHeader header;
    std::vector<FieldContainer> message_fields;
    PyMessageDatabase::ConstPtr parent_db = std::dynamic_pointer_cast<const PyMessageDatabase>(this->MessageDb());

    STATUS status = ReadIntermediate(message_data, header, message_fields, metadata);
    header.format = metadata.eFormat;

    nb::object pyinst;
    switch (status)
    {
    case STATUS::SUCCESS: return create_message_instance(header, message_fields, metadata, parent_db);
    case STATUS::NO_DEFINITION: return create_unknown_message_instance(nb::bytes(message_data.pucMessageBody, message_data.uiMessageBodyLength), header, parent_db);
    case STATUS::UNKNOWN: return nb::bytes(message_data.pucMessage, message_data.uiMessageLength);
    case STATUS::STREAM_EMPTY: throw nb ::stop_iteration();
    default: throw_exception_from_status(status);
    }
}

oem::PyMessageData oem::FileConversionIterator::Convert()
{
    MetaDataStruct metadata;
    MessageDataStruct message_data;
    PyHeader header;
    std::vector<FieldContainer> message_fields;
    while (true)
    {
        STATUS status = this->parser.Read(message_data, metadata);
        switch (status)
        {
        case STATUS::SUCCESS: return PyMessageData(message_data);
        case STATUS::UNKNOWN: continue;
        case STATUS::NO_DEFINITION: continue;
        case STATUS::STREAM_EMPTY: throw nb::stop_iteration("No more messages detected in buffer");
        default: throw_exception_from_status(status);
        }
    }
}

void init_novatel_file_parser(nb::module_& m)
{
    nb::class_<oem::PyFileParser>(m, "FileParser")
        .def(nb::init<const std::filesystem::path&>(), "file_path"_a)
        .def(nb::init<const std::filesystem::path&, const PyMessageDatabase::Ptr&>(), "file_path"_a, "message_db"_a)
        .def_prop_ro("logger", &oem::PyFileParser::GetLogger)
        .def("enable_framer_decoder_logging", &oem::PyFileParser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug,
             "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::PyFileParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::PyFileParser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::PyFileParser::GetDecompressRangeCmp, &oem::PyFileParser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::PyFileParser::GetReturnUnknownBytes, &oem::PyFileParser::SetReturnUnknownBytes)
        .def_prop_rw("filter", &oem::PyFileParser::GetFilter, &oem::PyFileParser::SetFilter)
        .def(
            "set_stream", [](oem::PyFileParser& self, nb::object stream) { return self.SetStream(std::make_shared<pystream::istream>(stream, 0)); },
            "input_stream"_a)
        .def("read", &oem::PyFileParser::PyRead)
        .def("__iter__", [](nb::handle_t<oem::PyFileParser> self) { return self; })
        .def("__next__", [](oem::PyFileParser& self) { return self.PyRead(); })
        .def("convert",
             [](oem::PyFileParser& self, ENCODE_FORMAT fmt) {
                 self.SetEncodeFormat(fmt);
                 return oem::FileConversionIterator(self);
             })
        .def("reset", &oem::PyFileParser::Reset)
        .def(
            "flush",
            [](oem::PyFileParser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                char buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush(reinterpret_cast<uint8_t*>(buffer), oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false)
        .def_prop_ro("internal_buffer",
                     [](const oem::PyFileParser& self) { return nb::bytes(self.GetInternalBuffer(), oem::Parser::uiParserInternalBufferSize); });

    nb::class_<oem::FileConversionIterator>(m, "FileConversionIterator")
        .def("__iter__", [](nb::handle_t<oem::FileConversionIterator> self) { return self; })
        .def("__next__", &oem::FileConversionIterator::Convert);
}
