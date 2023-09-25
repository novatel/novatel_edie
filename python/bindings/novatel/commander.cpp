#include "novatel_edie/decoders/oem/commander.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_commander(nb::module_& m)
{
    nb::class_<oem::Commander>(m, "Commander")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("open", &oem::Commander::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::Commander::GetLogger)
        .def(
            "encode",
            [](oem::Commander& commander, std::string command, ENCODE_FORMAT format) {
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t buf_size = MESSAGE_SIZE_MAX;
                STATUS status = commander.Encode(command.c_str(), command.length(), buffer, buf_size, format);
                return nb::make_tuple(nb::bytes(buffer, buf_size), status);
            },
            "abbrev_ascii_command"_a, "encode_format"_a);
}
