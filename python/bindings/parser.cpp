#include "novatel_edie/decoders/oem/parser.hpp"

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"
#include "parser.hpp"
#include "py_logger.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

nb::object oem::HandlePythonReadStatus(STATUS status_, MessageDataStruct& message_data_, oem::PyHeader& header_,
                                  std::vector<FieldContainer>& message_fields_, oem::MetaDataStruct& metadata_,
                                   PyMessageDatabase::ConstPtr database_)
{
    header_.format = metadata_.eFormat;
    switch (status_)
    {
    case STATUS::SUCCESS: return create_message_instance(header_, message_fields_, metadata_, database_);
    case STATUS::NO_DEFINITION:
        return create_unknown_message_instance(nb::bytes(message_data_.pucMessageBody, message_data_.uiMessageBodyLength), header_, database_);
    case STATUS::UNKNOWN: return create_unknown_bytes(nb::bytes(message_data_.pucMessage, message_data_.uiMessageLength));
    default: throw_exception_from_status(status_);
    }
}

nb::object oem::PyParser::PyRead(bool decode_incomplete)
{
    oem::MetaDataStruct metadata;
    MessageDataStruct message_data;
    oem::PyHeader header;
    std::vector<FieldContainer> message_fields;

    STATUS status = ReadIntermediate(message_data, header, message_fields, metadata, decode_incomplete);
    return HandlePythonReadStatus(status, message_data, header, message_fields, metadata,
                                  std::static_pointer_cast<const PyMessageDatabase>(MessageDb()));

}

nb::object oem::PyParser::PyIterRead()
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

nb::object oem::PyParser::PyConvert(ENCODE_FORMAT fmt, bool decode_incomplete)
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
        status = Read(message_data, metadata, decode_incomplete);
        if (status != STATUS::UNKNOWN && status != STATUS::NO_DEFINITION) { break; }
    }
    throw_exception_from_status(status);
    nb::object py_inst = nb::inst_alloc(py_type);
    PyMessageData* c_inst = nb::inst_ptr<PyMessageData>(py_inst);
    new (c_inst) PyMessageData(message_data);
    nb::inst_mark_ready(py_inst);
    return py_inst;
}

nb::object oem::ConversionIterator::PyIterConvert()
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

void init_novatel_parser(nb::module_& m)
{
    nb::class_<oem::PyParser>(m, "Parser")
        .def("__init__", [](oem::PyParser* t) { new (t) oem::PyParser(MessageDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "message_db"_a)
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::PyParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::PyParser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::PyParser::GetDecompressRangeCmp, &oem::PyParser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::PyParser::GetReturnUnknownBytes, &oem::PyParser::SetReturnUnknownBytes)
        .def_prop_rw("filter", &oem::PyParser::GetFilter, &oem::PyParser::SetFilter)
        .def("write",
             [](oem::PyParser& self, const nb::bytes& data) { return self.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()); })
        .def("read", &oem::PyParser::PyRead, "decode_incomplete_abbreviated"_a = false,
             nb::sig("def read(decode_incomplete_abbreviated=False) -> Message | UnknownMessage | UnknownBytes"),
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
        .def("__iter__", [](nb::handle_t<oem::PyParser> self) { return self; })
        .def("__next__", &oem::PyParser::PyIterRead, nb::sig("def __next__() -> Message | UnknownMessage | UnknownBytes"),
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
        .def("convert", &oem::PyParser::PyConvert, "fmt"_a, "decode_incomplete_abbreviated"_a = false)
        .def(
            "iter_convert", [](oem::PyParser& self, ENCODE_FORMAT fmt) { return oem::ConversionIterator(self, fmt); }, "fmt"_a)
        .def(
            "flush",
            [](oem::PyParser& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                uint8_t buffer[oem::Parser::uiParserInternalBufferSize];
                uint32_t count = self.Flush(buffer, oem::Parser::uiParserInternalBufferSize);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false)
        .def_prop_ro("internal_buffer",
                     [](const oem::Parser& self) { return nb::bytes(self.GetInternalBuffer(), oem::Parser::uiParserInternalBufferSize); });

    nb::class_<oem::ConversionIterator>(m, "ConversionIterator")
        .def("__iter__", [](nb::handle_t<oem::ConversionIterator> self) { return self; })
        .def("__next__", &oem::ConversionIterator::PyIterConvert);
}
