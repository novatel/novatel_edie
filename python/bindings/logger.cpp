#include "novatel_edie/common/logger.hpp"

#include "bindings_core.hpp"
#include "py_logger.hpp"

namespace nb = nanobind;
using namespace nb::literals;
namespace spd = spdlog;

void init_common_logger(nb::module_& m, nb::module_& internal_m)
{
    // replace with a PyLoggerManager
    pclLoggerManager.reset(new PyLoggerManager());
    auto manager = static_cast<PyLoggerManager*>(pclLoggerManager.get());

    m.def(
        "disable_internal_logging", [manager]() { manager->DisableInternalLogging(); },
        "Disable logging which originates from novatel_edie's native C++ code.");
    m.def(
        "enable_internal_logging", [manager]() { manager->EnableInternalLogging(); },
        "Enable logging which originates from novatel_edie's native C++ code.");

    internal_m.def("set_level", [manager](nb::handle logger, int level) {
        manager->SetLoggerLevel(logger, level);
    });

    internal_m.def("exit_cleanup", [manager]() { manager->Shutdown(); });

    manager->SetInternalMod(internal_m);

    nb::module_ atexit = nb::module_::import_("atexit");
    atexit.attr("register")(internal_m.attr("exit_cleanup"));
}
