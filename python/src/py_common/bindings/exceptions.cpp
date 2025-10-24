#include "py_common/exceptions.hpp"

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/json_db_reader.hpp"
#include "py_oem/bindings.hpp"
#include "py_common/bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

void py_oem::init_novatel_exceptions(nb::module_& m)
{
    nb::handle base = nb::exception<EdieException>(m, "NovatelEdieException");              // NOLINT
    nb::exception<FailureException>(m, "FailureException", base);                           // NOLINT
    nb::exception<UnknownException>(m, "UnknownException", base);                           // NOLINT
    nb::exception<IncompleteException>(m, "IncompleteException", base);                     // NOLINT
    nb::exception<IncompleteMoreDataException>(m, "IncompleteMoreDataException", base);     // NOLINT
    nb::exception<NullProvidedException>(m, "NullProvidedException", base);                 // NOLINT
    nb::exception<NoDatabaseException>(m, "NoDatabaseException", base);                     // NOLINT
    nb::exception<NoDefinitionException>(m, "NoDefinitionException", base);                 // NOLINT
    nb::exception<NoDefinitionEmbeddedException>(m, "NoDefinitionEmbeddedException", base); // NOLINT
    nb::exception<BufferFullException>(m, "BufferFullException", base);                     // NOLINT
    nb::exception<BufferEmptyException>(m, "BufferEmptyException", base);                   // NOLINT
    nb::exception<StreamEmptyException>(m, "StreamEmptyException", base);                   // NOLINT
    nb::exception<UnsupportedException>(m, "UnsupportedException", base);                   // NOLINT
    nb::exception<MalformedInputException>(m, "MalformedInputException", base);             // NOLINT
    nb::exception<DecompressionFailureException>(m, "DecompressionFailureException", base); // NOLINT
    nb::exception<JsonDbReaderFailure>(m, "JsonDbReaderException", base);                   // NOLINT

    nb::enum_<STATUS>(m, "STATUS", nb::is_arithmetic())
        .value("SUCCESS", STATUS::SUCCESS, "Successfully found a frame in the framer buffer.")
        .value("FAILURE", STATUS::FAILURE, "An unexpected failure occurred.")
        .value("UNKNOWN", STATUS::UNKNOWN, "Could not identify bytes as a protocol.")
        .value("INCOMPLETE", STATUS::INCOMPLETE, "It is possible that a valid frame exists in the frame buffer, but more information is needed.")
        .value("INCOMPLETE_MORE_DATA", STATUS::INCOMPLETE_MORE_DATA, "The current frame buffer is incomplete but more data is expected.")
        .value("NULL_PROVIDED", STATUS::NULL_PROVIDED, "A null pointer was provided.")
        .value("NO_DATABASE", STATUS::NO_DATABASE, "No database has been provided to the component.")
        .value("NO_DEFINITION", STATUS::NO_DEFINITION, "No definition could be found in the database for the provided message.")
        .value("NO_DEFINITION_EMBEDDED", STATUS::NO_DEFINITION_EMBEDDED,
               "No definition could be found in the database for the embedded message in the RXCONFIG log.")
        .value("BUFFER_FULL", STATUS::BUFFER_FULL, "The destination buffer is not big enough to contain the provided data.")
        .value("BUFFER_EMPTY", STATUS::BUFFER_EMPTY, "The internal circular buffer does not contain any unread bytes")
        .value("STREAM_EMPTY", STATUS::STREAM_EMPTY, "The input stream is empty.")
        .value("UNSUPPORTED", STATUS::UNSUPPORTED, "An attempted operation is unsupported by this component.")
        .value("MALFORMED_INPUT", STATUS::MALFORMED_INPUT, "The input is recognizable, but has unexpected formatting.")
        .value("DECOMPRESSION_FAILURE", STATUS::DECOMPRESSION_FAILURE, "The RANGECMPx log could not be decompressed.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    m.def("throw_exception_from_status", &throw_exception_from_status, "status"_a);
}
