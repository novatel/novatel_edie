#include "novatel_edie/common/logger.hpp"

#include "bindings_core.hpp"
#include "py_logger.hpp"

namespace nb = nanobind;
using namespace nb::literals;
namespace spd = spdlog;

void init_common_logger(nb::module_& m)
{
    // Shutdown existing CPPLoggerManager and replace with a PyLoggerManager
    GetLoggerManager()->Shutdown();
    pclLoggerManager.reset(new python_log::PyLoggerManager());
    auto manager = static_cast<python_log::PyLoggerManager*>(pclLoggerManager.get());

    m.def(
        "disable_internal_logging", [manager]() { manager->DisableInternalLogging(); },
        "Disable logging which originates in novatel_edie's native C++ code.");
    m.def(
        "enable_internal_logging", [manager]() { manager->EnableInternalLogging(); },
        "Enable logging which originates in novatel_edie's native C++ code.");
}
