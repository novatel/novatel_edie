#include "novatel_edie/decoders/oem/file_parser.hpp"

#include "bindings_core.hpp"
#include "file_parser.hpp"
#include "message_db_singleton.hpp"
#include "parser.hpp"
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

    STATUS status = ReadIntermediate(message_data, header, message_fields, metadata);

    return HandlePythonReadStatus(status, message_data, header, message_fields, metadata,
                                  std::static_pointer_cast<const PyMessageDatabase>(this->MessageDb()));
}

nb::object oem::PyFileParser::PyIterRead()
{
    try
    {
        return PyRead();
    }
    catch (StreamEmptyException)
    {
        throw nb::stop_iteration("No more messages detected in buffer");
    }
}

nb::object oem::PyFileParser::PyConvert(ENCODE_FORMAT fmt)
{
    static nb::handle py_type = nb::type<PyMessageData>();
    static oem::MetaDataStruct metadata;
    static MessageDataStruct message_data;
    static oem::PyHeader header;
    static std::vector<FieldContainer> message_fields;

    SetEncodeFormat(fmt);

    STATUS status;
    while (true)
    {
        status = Read(message_data, metadata);
        if (status != STATUS::UNKNOWN && status != STATUS::NO_DEFINITION) { break; }
    }
    throw_exception_from_status(status);
    nb::object py_inst = nb::inst_alloc(py_type);
    PyMessageData* c_inst = nb::inst_ptr<PyMessageData>(py_inst);
    new (c_inst) PyMessageData(message_data);
    nb::inst_mark_ready(py_inst);
    return py_inst;
}

nb::object oem::FileConversionIterator::PyIterConvert()
{
    try
    {
        return this->parser.PyConvert(fmt);
    }
    catch (StreamEmptyException)
    {
        throw nb::stop_iteration("No more messages detected in buffer");
    }
}

void init_novatel_file_parser(nb::module_& m)
{
    nb::class_<oem::PyFileParser>(m, "FileParser")
        .def(nb::init<const std::filesystem::path&>(), "file_path"_a)
        .def(nb::init<const std::filesystem::path&, const PyMessageDatabase::Ptr&>(), "file_path"_a, "message_db"_a)
        //.def_prop_ro("logger", &oem::PyFileParser::GetLogger)
        //.def("enable_framer_decoder_logging", &oem::PyFileParser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug,
        //     "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::PyFileParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::PyFileParser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::PyFileParser::GetDecompressRangeCmp, &oem::PyFileParser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::PyFileParser::GetReturnUnknownBytes, &oem::PyFileParser::SetReturnUnknownBytes)
        .def_prop_rw("filter", &oem::PyFileParser::GetFilter, &oem::PyFileParser::SetFilter)
        .def("read", &oem::PyFileParser::PyRead, nb::sig("def read() ->  Message | UnknownMessage | UnknownBytes"),
             R"doc(
            Attempts to read a message from data in the FileParser's buffer.

            Returns:
                A decoded `Message`,
                an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition,
                or a series of `UnknownBytes` determined to be undecodable.

            Raises:
                BufferEmptyException: There is insufficient data in the FileParser's
                buffer to decode a message.
            )doc")
        .def("__iter__", [](nb::handle_t<oem::PyFileParser> self) { return self; })
        .def("__next__", &oem::PyFileParser::PyIterRead, nb::sig("def __next__() -> Message | UnknownMessage | UnknownBytes"),
             R"doc(
            Attempts to read the next message from data in the FileParser's buffer.

            Returns:
                A decoded `Message`,
                an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition,
                or a series of `UnknownBytes` determined to be undecodable.

            Raises:
                StopIteration: There is insufficient data in the FileParser's
                buffer to decode a message.
            )doc")
        .def("convert", &oem::PyFileParser::PyConvert, "fmt"_a)
        .def("iter_convert", [](oem::PyFileParser& self, ENCODE_FORMAT fmt) { return oem::FileConversionIterator(self, fmt); })
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
        .def("__next__", &oem::FileConversionIterator::PyIterConvert);
}
