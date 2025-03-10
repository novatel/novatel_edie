#include "exceptions.hpp"

#include "bindings_core.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/json_db_reader.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void novatel::edie::throw_exception_from_status(STATUS status)
{
    switch (status)
    {
    case STATUS::SUCCESS:
        // No exception for success
        break;
    case STATUS::FAILURE: throw FailureException();
    case STATUS::UNKNOWN: throw UnknownException();
    case STATUS::INCOMPLETE: throw IncompleteException();
    case STATUS::INCOMPLETE_MORE_DATA: throw IncompleteMoreDataException();
    case STATUS::NULL_PROVIDED: throw NullProvidedException();
    case STATUS::NO_DATABASE: throw NoDatabaseException();
    case STATUS::NO_DEFINITION: throw NoDefinitionException();
    case STATUS::NO_DEFINITION_EMBEDDED: throw NoDefinitionEmbeddedException();
    case STATUS::BUFFER_FULL: throw BufferFullException();
    case STATUS::BUFFER_EMPTY: throw BufferEmptyException();
    case STATUS::STREAM_EMPTY: throw StreamEmptyException();
    case STATUS::UNSUPPORTED: throw UnsupportedException();
    case STATUS::MALFORMED_INPUT: throw MalformedInputException();
    case STATUS::DECOMPRESSION_FAILURE: throw DecompressionFailureException();
    default: throw std::runtime_error("Unknown STATUS value: " + std::to_string(int(status)));
    }
}

void init_novatel_exceptions(nb::module_& m)
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

    m.def("throw_exception_from_status", &throw_exception_from_status, "status"_a);
}
