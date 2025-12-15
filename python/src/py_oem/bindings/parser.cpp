#include "novatel_edie/decoders/oem/parser.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_common/py_logger.hpp"
#include "py_common/py_message_data.hpp"
#include "py_common/unknown_bytes.hpp"
#include "py_oem/filter.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/message_database.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/parser.hpp"
#include "py_oem/py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

nb::object py_oem::HandlePythonReadStatus(STATUS status_, MessageDataStruct& message_data_, py_oem::PyHeader& header_,
                                          std::vector<FieldContainer>&& message_fields_, oem::MetaDataStruct& metadata_,
                                          py_oem::PyMessageDatabase::ConstPtr database_)
{
    header_.format = metadata_.eFormat;
    switch (status_)
    {
    case STATUS::SUCCESS: return create_message_instance(header_, std::move(message_fields_), metadata_, database_);
    case STATUS::NO_DEFINITION:
        return create_unknown_message_instance(nb::bytes(message_data_.pucMessageBody, message_data_.uiMessageBodyLength), header_, database_);
    case STATUS::UNKNOWN: return py_common::create_unknown_bytes(nb::bytes(message_data_.pucMessage, message_data_.uiMessageLength));
    default: throw_exception_from_status(status_);
    }
}

nb::object py_oem::PyParser::PyRead(bool decode_incomplete)
{
    static oem::MetaDataStruct metadata;
    static MessageDataStruct message_data;
    py_oem::PyHeader header;
    std::vector<FieldContainer> message_fields;

    STATUS status = ReadIntermediate(message_data, header, message_fields, metadata, decode_incomplete);
    return HandlePythonReadStatus(status, message_data, header, std::move(message_fields), metadata, pclPyMessageDb);
}

nb::object py_oem::PyParser::PyIterRead()
{
    try
    {
        return PyRead(false);
    }
    catch (BufferEmptyException)
    {
        throw nb::stop_iteration("No more messages detected in buffer");
    }
}

nb::object py_oem::PyParser::PyConvert(ENCODE_FORMAT fmt, bool decode_incomplete)
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
        status = Read(message_data, metadata, decode_incomplete);
        if (status != STATUS::UNKNOWN && status != STATUS::NO_DEFINITION) { break; }
    }
    throw_exception_from_status(status);
    nb::object py_inst = nb::inst_alloc(py_type);
    py_common::PyMessageData* c_inst = nb::inst_ptr<py_common::PyMessageData>(py_inst);
    new (c_inst) py_common::PyMessageData(message_data);
    nb::inst_mark_ready(py_inst);
    return py_inst;
}

nb::object py_oem::ConversionIterator::PyIterConvert()
{
    try
    {
        return this->parser.PyConvert(fmt, false);
    }
    catch (BufferEmptyException)
    {
        throw nb::stop_iteration("No more messages detected in buffer");
    }
}

void py_oem::init_novatel_parser(nb::module_& m)
{
    nb::class_<py_oem::PyParser>(m, "Parser")
        .def(
            "__init__",
            [](py_oem::PyParser* self, py_common::PyMessageDatabaseCore::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); }
                new (self) py_oem::PyParser(message_db);
            },
            nb::arg("message_db") = nb::none(),
            R"doc(
             Initializes a Parser.

             Args:
                 message_db: The message database to parse messages with.
                    If None, use the default database.
            )doc")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &py_oem::PyParser::GetIgnoreAbbreviatedAsciiResponses,
                     &py_oem::PyParser::SetIgnoreAbbreviatedAsciiResponses,
                     "Whether to skip over abbreviated ascii message responses e.g. `<OK`/`<ERROR`.")
        .def_prop_rw("decompress_range_cmp", &py_oem::PyParser::GetDecompressRangeCmp, &py_oem::PyParser::SetDecompressRangeCmp,
                     "Whether to decompress compressed RANGE messages.")
        .def_prop_rw("return_unknown_bytes", &py_oem::PyParser::GetReturnUnknownBytes, &py_oem::PyParser::SetReturnUnknownBytes,
                     "Whether to return unidentifiable data.")
        .def_prop_rw(
            "filter",
            [](py_oem::PyParser& self) {
                // This static cast is safe so long as the Parser's filter is set only via the Python interface
                return std::static_pointer_cast<oem::PyFilter>(self.GetFilter());
            },
            [](py_oem::PyParser& self, oem::PyFilter::Ptr filter) { self.SetFilter(filter); },
            "The filter which controls which data is skipped over.")
        .def_prop_ro("available_space", &py_oem::PyParser::GetAvailableSpace,
                     "The number of bytes in the Parser's internal buffer available for writing new data.")
        .def(
            "write",
            [](py_oem::PyParser& self, const nb::bytes& data) { return self.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()); },
            R"doc(
             Writes data to the Parser's internal buffer allowing it to later be parsed.

             Use 'available_space' attribute to check how many bytes can be safely written.

             Args:
                 data: A set of bytes to append to the Parser's internal buffer.

             Returns:
                    The number of bytes written to the Parser's internal buffer.
                    Can be less than the length of `data` if the buffer is full.
            )doc")
        .def("read", &py_oem::PyParser::PyRead, "decode_incomplete_abbreviated"_a = false,
             nb::sig("def read(self, decode_incomplete_abbreviated: bool = False) -> Message | Response | UnknownMessage | UnknownBytes"),
             R"doc(
            Attempts to read a message from data in the Parser's buffer.

            Args:
                decode_incomplete_abbreviated: If True, the Parser will try to
                interpret a possibly incomplete abbreviated ASCII message as if
                it were complete. This is necessary when there is no data
                following the message to indicate that its end.

            Returns:
                A decoded `Message`,
                an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition,
                or a series of `UnknownBytes` determined to be undecodable.

            Raises:
                BufferEmptyException: There is insufficient data in the Parser's
                buffer to decode a message.
            )doc")
        .def(
            "__iter__", [](nb::handle_t<py_oem::PyParser> self) { return self; },
            nb::sig("def __iter__(self) -> Iterator[Message|Response|UnknownMessage|UnknownBytes]"),
            R"doc(
            Marks Parser as Iterable.

            Returns:
                The Parser itself as an Iterator.
            )doc")
        .def("__next__", &py_oem::PyParser::PyIterRead, nb::sig("def __next__(self) -> Message | Response | UnknownMessage | UnknownBytes"),
             R"doc(
            Attempts to read the next message from data in the Parser's buffer.

            Returns:
                A decoded `Message`,
                an `UnknownMessage` whose header was identified but whose payload
                could not be decoded due to no available message definition,
                or a series of `UnknownBytes` determined to be undecodable.

            Raises:
                StopIteration: There is insufficient data in the Parser's
                buffer to decode a message.
            )doc")
        .def("convert", &py_oem::PyParser::PyConvert, "fmt"_a, "decode_incomplete_abbreviated"_a = false,
             nb::sig("def convert(fmt: ENCODE_FORMAT, decode_incomplete_abbreviated: bool = False) -> MessageData"),
             R"doc(
            Converts the next message in the buffer to the specified format.

            Args:
                fmt: The format to convert the message to.
                decode_incomplete_abbreviated: If True, the Parser will try to
                    interpret a possibly incomplete abbreviated ASCII message as if
                    it were complete. This is necessary when there is no data
                    following the message to indicate its end.

            Returns:
                The converted message.

            Raises:
                BufferEmptyException: There is insufficient data in the Parser's
                buffer to decode a message.
            )doc")
        .def(
            "iter_convert", [](py_oem::PyParser& self, ENCODE_FORMAT fmt) { return py_oem::ConversionIterator(self, fmt); }, "fmt"_a,
            R"doc(
            Creates an interator which parses and converts messages to a specified format.

            Args:
                fmt: The format to convert messages to.

            Returns:
                An iterator that directly converts messages.
            )doc")
        .def(
            "flush",
            [](py_oem::PyParser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                uint8_t buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush(buffer, oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false,
            R"doc(
            Flushes all bytes from the internal Parser.

            Args:
                return_flushed_bytes: If True, the flushed bytes will be returned.

            Returns:
                The number of bytes flushed if return_flushed_bytes is False,
                otherwise the flushed bytes.
            )doc");

    nb::class_<py_oem::ConversionIterator>(m, "ConversionIterator")
        .def(
            "__iter__", [](nb::handle_t<py_oem::ConversionIterator> self) { return self; }, nb::sig("def __iter__(self) -> Iterator[MessageData]"),
            R"doc(
            Marks ConversionIterator as Iterable.

            Returns:
                The ConversionIterator itself as an Iterator.
            )doc")
        .def("__next__", &py_oem::ConversionIterator::PyIterConvert, nb::sig("def __next__() -> MessageData"),
             R"doc(
            Converts the next message in the buffer to the specified format.

            Returns:
                The converted message.

            Raises:
                StopIteration: There is insufficient data in the Parser's
                buffer to decode a message.
            )doc");
}
