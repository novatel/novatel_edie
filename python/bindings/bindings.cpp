#include <nanobind/nanobind.h>
#include <memory>

#include "novatel_edie/common/logger.hpp"

namespace nb = nanobind;

void init_common_common(nb::module_&);
void init_common_logger(nb::module_&, nb::module_&);
void init_message_db_singleton(nb::module_&);
void init_common_message_database(nb::module_&);
void init_novatel_commander(nb::module_&);
void init_novatel_common(nb::module_&);
void init_novatel_exceptions(nb::module_&);
void init_novatel_decoder(nb::module_&);
void init_novatel_file_parser(nb::module_&);
void init_novatel_filter(nb::module_&);
void init_novatel_framer(nb::module_&);
void init_header_objects(nb::module_&);
void init_message_objects(nb::module_&);
void init_novatel_oem_enums(nb::module_&);
void init_novatel_oem_messages(nb::module_&);
void init_novatel_parser(nb::module_&);
void init_novatel_range_decompressor(nb::module_&);
void init_novatel_rxconfig_handler(nb::module_&);
void init_decoder_tester(nb::module_&);
void init_logger_tester(nb::module_&);

NB_MODULE(bindings, m)
{
    nb::module_ messages_mod = m.def_submodule("messages", "NovAtel OEM message definitions.");
    nb::module_ enums_mod = m.def_submodule("enums", "Enumerations used by NovAtel OEM message fields.");
    nb::module_ internal_mod = m.def_submodule("_internal", "NOT PART OF THE PUBLIC API.");

    init_common_common(m);
    init_common_logger(m, internal_mod);
    init_message_db_singleton(m);
    init_novatel_commander(m);
    init_novatel_common(m);
    init_novatel_exceptions(m);
    init_novatel_file_parser(m);
    init_novatel_filter(m);
    init_novatel_framer(m);
    init_novatel_decoder(m);
    init_header_objects(m);
    init_message_objects(m);
    init_common_message_database(m);
    init_novatel_parser(m);
    init_novatel_rxconfig_handler(m);
    init_novatel_range_decompressor(m);
    init_novatel_oem_messages(messages_mod);
    init_novatel_oem_enums(enums_mod);
    init_decoder_tester(internal_mod);
    init_logger_tester(internal_mod);
}
