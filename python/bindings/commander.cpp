#include "novatel_edie/decoders/oem/commander.hpp"

#include "bindings_core.hpp"
#include "exceptions.hpp"
#include "message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_commander(nb::module_& m)
{
    nb::class_<oem::Commander>(m, "Commander")
        .def(
            "__init__",
            [](oem::Commander* t, PyMessageDatabase::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); };
                new (t) oem::Commander(message_db->GetCoreDatabase());
            },
            nb::arg("message_db") = nb::none()) // NOLINT(*.NewDeleteLeaks)
        .def("load_db", &oem::Commander::LoadJsonDb, "message_db"_a)
        .def(
            "encode",
            [](oem::Commander& commander, const nb::bytes& command, const ENCODE_FORMAT format) {
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                STATUS status = commander.Encode(command.c_str(), nb::len(command), buffer, buf_size, format);
                throw_exception_from_status(status);
                return nb::bytes(buffer, buf_size);
            },
            "abbrev_ascii_command"_a, "encode_format"_a);
}
