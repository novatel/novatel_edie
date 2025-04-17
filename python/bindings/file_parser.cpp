#include "novatel_edie/decoders/oem/file_parser.hpp"

#include "bindings_core.hpp"
#include "file_parser.hpp"
#include "filter.hpp"
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
    static MetaDataStruct metadata;
    static MessageDataStruct message_data;
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
        .def(
            "__init__",
            [](oem::PyFileParser* self, const std::filesystem::path& file_path, PyMessageDatabase::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); }
                new (self) oem::PyFileParser(file_path, message_db);
            },
            "file_path"_a,
            nb::arg("message_db") = nb::none(),
            R"doc(
             Initializes a FileParser.

             Args:
                 file_path: The path to the file to be parsed.
                 message_db: The message database to parse message with.
                    If None, use the default database.
            )doc")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::PyFileParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::PyFileParser::SetIgnoreAbbreviatedAsciiResponses,
                     "Whether to skip over abbreviated ASCII message responses e.g. `<OK`/`<ERROR`.")
        .def_prop_rw("decompress_range_cmp", &oem::PyFileParser::GetDecompressRangeCmp, &oem::PyFileParser::SetDecompressRangeCmp,
                     "Whether to decompress compressed RANGE messages.")
        .def_prop_rw("return_unknown_bytes", &oem::PyFileParser::GetReturnUnknownBytes, &oem::PyFileParser::SetReturnUnknownBytes,
                     "Whether to return unidentifiable data.")
        .def_prop_rw(
            "filter",
            [](oem::PyFileParser& self) {
                // This static cast is safe so long as the FileParser's filter is set only via the Python interface
                return std::static_pointer_cast<oem::PyFilter>(self.GetFilter());
            },
            [](oem::PyFileParser& self, oem::PyFilter::Ptr filter) { self.SetFilter(filter); },
            "The filter which controls which data is skipped over."
            )
        .def("read", &oem::PyFileParser::PyRead, nb::sig("def read() ->  Message | UnknownMessage | UnknownBytes"),
             R"doc(
            Attempts to read a message from remaining data in the file.

            Returns:
                A decoded `Message`,
                an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition,
                or a series of `UnknownBytes` determined to be undecodable.

            Raises:
                StreamEmptyException: There is insufficient data in the remaining 
                    in the file to decode a message.
            )doc")
        .def("__iter__", [](nb::handle_t<oem::PyFileParser> self) { return self; },
             R"doc(
            Marks FileParser as Iterable.

            Returns:
                The FileParser itself as an Iterator.
            )doc")
        .def("__next__", &oem::PyFileParser::PyIterRead, nb::sig("def __next__() -> Message | UnknownMessage | UnknownBytes"),
             R"doc(
            Attempts to read the next message from remaining data in the file.

            Returns:
                A decoded `Message`,
                an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition,
                or a series of `UnknownBytes` determined to be undecodable.

            Raises:
                StopIteration: There is insufficient data in the remaining
                    file to decode a message.
            )doc")
        .def("convert", &oem::PyFileParser::PyConvert, "fmt"_a,
             R"doc(
            Converts the next message in the file to the specified format.

            Args:
                fmt: The format to convert the message to.

            Returns:
                The converted message.

            Raises:
                StreamEmptyException: There is insufficient data in the remaining 
                    in the file to decode a message.
            )doc")
        .def("iter_convert", [](oem::PyFileParser& self, ENCODE_FORMAT fmt) { return oem::FileConversionIterator(self, fmt); }, "fmt"_a,
             R"doc(
            Creates an iterator which parses and converts messages to a specified format.

            Args:
                fmt: The format to convert messages to.

            Returns:
                An iterator that directly converts messages.
            )doc")
        .def("reset", &oem::PyFileParser::Reset,
             R"doc(
            Resets the FileParser, clearing its internal state.
            )doc")
        .def(
            "flush",
            [](oem::PyFileParser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                char buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush(reinterpret_cast<uint8_t*>(buffer), oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false,
            R"doc(
            Flushes all bytes from the FileParser.

            Args:
                return_flushed_bytes: If True, the flushed bytes will be returned.

            Returns:
                The number of bytes flushed if return_flushed_bytes is False,
                otherwise the flushed bytes.
            )doc");

    nb::class_<oem::FileConversionIterator>(m, "FileConversionIterator")
        .def("__iter__", [](nb::handle_t<oem::FileConversionIterator> self) { return self; },
             R"doc(
            Marks FileConversionIterator as Iterable.

            Returns:
                The FileConversionIterator itself as an Iterator.
            )doc")
        .def("__next__", &oem::FileConversionIterator::PyIterConvert, nb::sig("def __next__() -> MessageData"),
             R"doc(
            Converts the next message in the file to the specified format.

            Returns:
                The converted message.

            Raises:
                StopIteration: There is insufficient data in the remaining
                    file to decode a message.
            )doc");
}
