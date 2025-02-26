#include "novatel_edie/decoders/oem/parser.hpp"

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"
#include "parser.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

nb::object oem::PyParser::PyRead(bool decode_incomplete) {
    oem::MetaDataStruct metadata;
    MessageDataStruct message_data;
    oem::PyHeader header;
    std::vector<FieldContainer> message_fields;
    PyMessageDatabase::ConstPtr parent_db = std::dynamic_pointer_cast<const PyMessageDatabase>(MessageDb());
    STATUS status = ReadIntermediate(message_data, header, message_fields, metadata, decode_incomplete);
    header.format = metadata.eFormat;

    nb::object pyinst;
    switch (status)
    {
    case STATUS::SUCCESS: pyinst = create_message_instance(header, message_fields, metadata, parent_db); break;
    case STATUS::NO_DEFINITION:
        pyinst = create_unknown_message_instance(nb::bytes(message_data.pucMessageBody, message_data.uiMessageBodyLength), header, parent_db);
        break;
    case STATUS::UNKNOWN: pyinst = nb::bytes(message_data.pucMessage, message_data.uiMessageLength); break;
    case STATUS::BUFFER_EMPTY: throw nb::stop_iteration("No more messages detected in buffer");
    default: throw_exception_from_status(status);
    }
    return pyinst;
}

oem::PyMessageData oem::ConversionIterator::Convert(bool decode_incomplete)
{
    oem::MetaDataStruct metadata;
    MessageDataStruct message_data;
    oem::PyHeader header;
    std::vector<FieldContainer> message_fields;
    while (true)
    {
        STATUS status = this->parser.Read(message_data, metadata, decode_incomplete);
        switch (status)
        {
        case STATUS::SUCCESS: return PyMessageData(message_data);
        case STATUS::UNKNOWN: continue;
        case STATUS::NO_DEFINITION: continue;
        case STATUS::BUFFER_EMPTY: throw nb::stop_iteration("No more messages detected in buffer");
        default: throw_exception_from_status(status);
        }
    }
}

void init_novatel_parser(nb::module_& m)
{
    nb::class_<oem::PyParser>(m, "Parser")
        .def("__init__", [](oem::PyParser* t) { new (t) oem::PyParser(MessageDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "message_db"_a)
        .def_prop_ro("logger", &oem::PyParser::GetLogger)
        .def("enable_framer_decoder_logging", &oem::PyParser::EnableFramerDecoderLogging, "level"_a = spdlog::level::debug, "filename"_a = "edie.log")
        .def_prop_rw("ignore_abbreviated_ascii_responses", &oem::PyParser::GetIgnoreAbbreviatedAsciiResponses,
                     &oem::PyParser::SetIgnoreAbbreviatedAsciiResponses)
        .def_prop_rw("decompress_range_cmp", &oem::PyParser::GetDecompressRangeCmp, &oem::PyParser::SetDecompressRangeCmp)
        .def_prop_rw("return_unknown_bytes", &oem::PyParser::GetReturnUnknownBytes, &oem::PyParser::SetReturnUnknownBytes)
        .def_prop_rw("filter", &oem::PyParser::GetFilter, &oem::PyParser::SetFilter)
        .def("write",
             [](oem::PyParser& self, const nb::bytes& data) { return self.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()); })
        .def("read", &oem::PyParser::PyRead, "decode_incomplete_abbreviated"_a = false)
        .def("__iter__", [](nb::handle_t<oem::PyParser> self) { return self; })
        .def("__next__", [](oem::PyParser& self) { return self.PyRead(false);})
        .def("convert",
             [](oem::PyParser& self, ENCODE_FORMAT fmt) {
                 self.SetEncodeFormat(fmt);
                 return oem::ConversionIterator(self);
             })
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
        .def("__next__", &oem::ConversionIterator::Convert, "decode_incomplete_abbreviated"_a = false);
}
