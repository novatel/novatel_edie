#include "py_oem/bindings.hpp"

#include <memory>

#include <nanobind/nanobind.h>

#include "novatel_edie/common/logger.hpp"

namespace nb = nanobind;
using namespace novatel::edie::py_oem;


NB_MODULE(bindings, m)
{
    nb::module_ internal_mod = m.def_submodule("_internal", "NOT PART OF THE PUBLIC API.");
    nb::module_ messages_mod = m.def_submodule("messages", "NovAtel OEM message types.");
    nb::module_ enums_mod = m.def_submodule("enums", "Enumerations used by NovAtel OEM message fields.");

    init_common_common(m);
    init_common_logger(m, internal_mod);
    init_common_message_database(m);
    init_message_db_singleton(m);
    init_header_objects(m);
    init_message_objects(m);
    init_novatel_common(m);
    init_novatel_exceptions(m);
    init_novatel_parser(m);
    init_novatel_file_parser(m);
    init_novatel_framer(m);
    init_novatel_decoder(m);
    init_novatel_filter(m);
    init_novatel_commander(m);
    init_novatel_range_decompressor(m);
    init_novatel_rxconfig_handler(m);
    init_novatel_oem_messages(messages_mod);
    init_novatel_oem_enums(enums_mod);
    init_decoder_tester(internal_mod);
    init_logger_tester(internal_mod);
}
