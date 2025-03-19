#include "novatel_edie/common/logger.hpp"

#include "bindings_core.hpp"
#include "py_logger.hpp"

namespace nb = nanobind;
using namespace nb::literals;
namespace spd = spdlog;

void init_common_logger(nb::module_& m)
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
}
