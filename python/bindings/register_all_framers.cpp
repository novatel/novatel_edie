#include "register_all_framers.hpp"
#include "novatel_edie/decoders/common/framer_registration.hpp"


void init_register_all_framers(nb::module_& m) { m.def("register_all_framers", &RegisterAllFramers, "Registers all built-in Framer types."); }
