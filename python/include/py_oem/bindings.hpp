#pragma once

#include "py_common/bindings_core.hpp"


namespace nb = nanobind;

namespace novatel::edie::py_oem {
void init_header_objects(nb::module_&);
void init_message_objects(nb::module_&);
void init_novatel_common(nb::module_&);
void init_novatel_parser(nb::module_&);
void init_novatel_file_parser(nb::module_&);
void init_novatel_framer(nb::module_&);
void init_novatel_decoder(nb::module_&);
void init_novatel_filter(nb::module_&);
void init_novatel_commander(nb::module_&);
void init_novatel_range_decompressor(nb::module_&);
void init_novatel_rxconfig_handler(nb::module_&);
void init_novatel_oem_messages(nb::module_&);
void init_novatel_oem_enums(nb::module_&);
void init_decoder_tester(nb::module_&);
} // namespace novatel::edie::py_oem
