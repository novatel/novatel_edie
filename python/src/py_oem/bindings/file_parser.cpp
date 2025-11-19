#include "novatel_edie/decoders/oem/file_parser.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/message_db_singleton.hpp"
#include "py_common/py_message_data.hpp"
#include "py_common/pystream.hpp"
#include "py_oem/file_parser.hpp"
#include "py_oem/filter.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/parser.hpp"
#include "py_oem/py_message_objects.hpp"


namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

nb::object py_oem::PyFileParser::PyRead()
{
    static oem::MetaDataStruct metadata;
    static MessageDataStruct message_data;
    py_oem::PyHeader header;
    std::vector<FieldContainer> message_fields;

    STATUS status = ReadIntermediate(message_data, header, message_fields, metadata);

    return HandlePythonReadStatus(status, message_data, header, std::move(message_fields), metadata, pclPyMessageDb);
}

nb::object py_oem::PyFileParser::PyIterRead()
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

nb::object py_oem::PyFileParser::PyConvert(ENCODE_FORMAT fmt)
{
    static nb::handle py_type = nb::type<py_common::PyMessageData>();
    static oem::MetaDataStruct metadata;
    static MessageDataStruct message_data;
    static py_oem::PyHeader header;
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
    py_common::PyMessageData* c_inst = nb::inst_ptr<py_common::PyMessageData>(py_inst);
    new (c_inst) py_common::PyMessageData(message_data);
    nb::inst_mark_ready(py_inst);
    return py_inst;
}

nb::object py_oem::FileConversionIterator::PyIterConvert()
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

void py_oem::init_novatel_file_parser(nb::module_& m)
{
    nb::class_<py_oem::PyFileParser>(m, "FileParser")
        .def(
            "__init__",
            [](py_oem::PyFileParser* self, const std::filesystem::path& file_path, py_common::PyMessageDatabaseCore::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); }
                new (self) py_oem::PyFileParser(file_path, message_db);
            },
            "file_path"_a, nb::arg("message_db") = nb::none(),
            R"doc(
             Initializes a FileParser.

             Args:
                 file_path: The path to the file to be parsed.
                 message_db: The message database to parse message with.
                    If None, use the default database.
            )doc")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &py_oem::PyFileParser::GetIgnoreAbbreviatedAsciiResponses,
                     &py_oem::PyFileParser::SetIgnoreAbbreviatedAsciiResponses,
                     "Whether to skip over abbreviated ASCII message responses e.g. `<OK`/`<ERROR`.")
        .def_prop_rw("decompress_range_cmp", &py_oem::PyFileParser::GetDecompressRangeCmp, &py_oem::PyFileParser::SetDecompressRangeCmp,
                     "Whether to decompress compressed RANGE messages.")
        .def_prop_rw("return_unknown_bytes", &py_oem::PyFileParser::GetReturnUnknownBytes, &py_oem::PyFileParser::SetReturnUnknownBytes,
                     "Whether to return unidentifiable data.")
        .def_prop_rw(
            "filter",
            [](py_oem::PyFileParser& self) {
                // This static cast is safe so long as the FileParser's filter is set only via the Python interface
                return std::static_pointer_cast<oem::PyFilter>(self.GetFilter());
            },
            [](py_oem::PyFileParser& self, oem::PyFilter::Ptr filter) { self.SetFilter(filter); },
            "The filter which controls which data is skipped over.")
        .def("read", &py_oem::PyFileParser::PyRead, nb::sig("def read(self) ->  Message | Response | UnknownMessage | UnknownBytes"),
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
        .def(
            "__iter__", [](nb::handle_t<py_oem::PyFileParser> self) { return self; },
            nb::sig("def __iter__(self) -> Iterator[Message|Response|UnknownMessage|UnknownBytes]"),
            R"doc(
            Marks FileParser as Iterable.

            Returns:
                The FileParser itself as an Iterator.
            )doc")
        .def("__next__", &py_oem::PyFileParser::PyIterRead, nb::sig("def __next__(self) -> Message | Response | UnknownMessage | UnknownBytes"),
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
        .def("convert", &py_oem::PyFileParser::PyConvert, "fmt"_a,
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
        .def(
            "iter_convert", [](py_oem::PyFileParser& self, ENCODE_FORMAT fmt) { return py_oem::FileConversionIterator(self, fmt); }, "fmt"_a,
            R"doc(
            Creates an iterator which parses and converts messages to a specified format.

            Args:
                fmt: The format to convert messages to.

            Returns:
                An iterator that directly converts messages.
            )doc")
        .def("reset", &py_oem::PyFileParser::Reset,
             R"doc(
            Resets the FileParser, clearing its internal state.
            )doc")
        .def(
            "flush",
            [](py_oem::PyFileParser& self, bool return_flushed_bytes) -> nb::object {
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

    nb::class_<py_oem::FileConversionIterator>(m, "FileConversionIterator")
        .def(
            "__iter__", [](nb::handle_t<py_oem::FileConversionIterator> self) { return self; },
            nb::sig("def __iter__(self) -> Iterator[MessageData]"),
            R"doc(
            Marks FileConversionIterator as Iterable.

            Returns:
                The FileConversionIterator itself as an Iterator.
            )doc")
        .def("__next__", &py_oem::FileConversionIterator::PyIterConvert, nb::sig("def __next__() -> MessageData"),
             R"doc(
            Converts the next message in the file to the specified format.

            Returns:
                The converted message.

            Raises:
                StopIteration: There is insufficient data in the remaining
                    file to decode a message.
            )doc");
}
