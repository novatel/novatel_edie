#include "novatel_edie/decoders/oem/command_decoder.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_oem/init_bindings.hpp"
#include "py_oem/message_database.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

namespace {

struct PyCommandDecoder
{
    PyMessageDatabase::Ptr database;
    oem::CommandDecoder command_decoder;

    explicit PyCommandDecoder(PyMessageDatabase::Ptr pclMessageDb_) : database(std::move(pclMessageDb_)), command_decoder(database->core()) {}
};

} // namespace

void py_oem::init_novatel_command_decoder(nb::module_& m)
{
    nb::class_<PyCommandDecoder>(m, "CommandDecoder")
        .def(
            "__init__",
            [](PyCommandDecoder* self, PyMessageDatabase::Ptr message_db) {
                if (!message_db) { message_db = py_oem::MessageDbSingleton::get(); }
                message_db->Lock();
                new (self) PyCommandDecoder(std::move(message_db));
            },
            nb::arg("message_db") = nb::none(),
            R"doc(
            Initializes a CommandDecoder.

            Args:
                message_db: The message database to decode commands with.
                    If None, use the default database.
            )doc")
        .def(
            "decode",
            [](const PyCommandDecoder& self, const nb::bytes& command) {
                py_oem::PyHeader header;
                CompositeField fields;
                STATUS status = self.command_decoder.Decode(command.c_str(), static_cast<uint32_t>(nb::len(command)), header, fields);
                throw_exception_from_status(status);

                oem::MetaDataStruct metadata;
                metadata.eFormat = DECODE_FORMAT::ABB_ASCII;
                metadata.usMessageId = header.usMessageId;
                metadata.uiMessageCrc = header.uiMessageDefinitionCrc;
                return py_oem::create_message_instance(header, std::move(fields), metadata, self.database);
            },
            "command"_a, nb::sig("def decode(self, command: bytes) -> Message"),
            R"doc(
            Decode an abbreviated ASCII command.

            Parameters left out of the command are filled in from the database, as the
            receiver does. An enum field with a default that the next parameter is not a
            value of takes its default, and fields left off the end take their defaults.

            Args:
                command: The abbreviated ASCII command, e.g. b"LOG BESTPOSA ONCE".

            Returns:
                The command as a `Message`, which can be encoded with `to_ascii()` or
                `to_binary()`.
            )doc");
}
