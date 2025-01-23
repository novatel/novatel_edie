#include <nanobind/nanobind.h>

namespace nb = nanobind;

void init_common_common(nb::module_&);
void init_common_logger(nb::module_&);
void init_common_json_db_reader(nb::module_&);
void init_common_message_database(nb::module_&);
void init_common_nexcept(nb::module_&);
void init_novatel_commander(nb::module_&);
void init_novatel_common(nb::module_&);
void init_novatel_encoder(nb::module_&);
void init_novatel_file_parser(nb::module_&);
void init_novatel_filter(nb::module_&);
void init_novatel_framer(nb::module_&);
void init_novatel_header_decoder(nb::module_&);
void init_novatel_message_decoder(nb::module_&);
void init_novatel_oem_enums(nb::module_&);
void init_novatel_oem_messages(nb::module_&);
void init_novatel_parser(nb::module_&);
void init_novatel_range_decompressor(nb::module_&);
void init_novatel_rxconfig_handler(nb::module_&);

NB_MODULE(bindings, m)
{
    nb::module_ classes_mod = m.def_submodule("classes", "Classes of the module.");
    init_common_common(classes_mod);
    init_common_logger(classes_mod);
    init_common_json_db_reader(classes_mod);
    init_common_nexcept(classes_mod);
    init_novatel_commander(classes_mod);
    init_novatel_common(classes_mod);
    init_novatel_encoder(classes_mod);
    init_novatel_file_parser(classes_mod);
    init_novatel_filter(classes_mod);
    init_novatel_framer(classes_mod);
    init_novatel_header_decoder(classes_mod);
    init_novatel_message_decoder(classes_mod);
    init_common_message_database(classes_mod);
    init_novatel_parser(classes_mod);
    init_novatel_rxconfig_handler(classes_mod);
    init_novatel_range_decompressor(classes_mod);
    nb::module_ messages_mod = m.def_submodule("messages", "NovAtel OEM message definitions.");
    init_novatel_oem_messages(messages_mod);
    nb::module_ enums_mod = m.def_submodule("enums", "Enumerations used by NovAtel OEM message fields.");
    init_novatel_oem_enums(enums_mod);
}
