#include "py_common/init_bindings.hpp"

#include "novatel_edie/common/logger.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/py_logger.hpp"

namespace nb = nanobind;
using namespace nb::literals;
namespace spd = spdlog;
using namespace novatel::edie;

void py_common::init_common_logger(nb::module_& m, nb::module_& internal_m)
{
    // Set the global LoggerManager to be a PyLoggerManager
    SetLoggerManager(new PyLoggerManager());
    auto manager = static_cast<PyLoggerManager*>(GetBaseLoggerManager());

    // Add public functions for configuring internal logging
    m.def(
        "disable_internal_logging", [manager]() { manager->DisableInternalLogging(); },
        "Disable logging which originates from novatel_edie's native C++ code.");
    m.def(
        "enable_internal_logging", [manager]() { manager->EnableInternalLogging(); },
        "Enable logging which originates from novatel_edie's native C++ code.");

    // Create python entry-point to custom setLevel and cleanup functions
    internal_m.def("set_level", [manager](nb::handle self, nb::args args_, nb::kwargs kwargs_) { manager->SetLoggerLevel(self, args_, kwargs_); });
    internal_m.def("exit_cleanup", [manager]() { manager->Shutdown(); });

    // Provide these the setLevel entry-point to the LoggerManager
    manager->SetInternalMod(internal_m);

    // Run the custom cleanup code before any normal termination of the Python interpreter
    nb::module_ atexit = nb::module_::import_("atexit");
    atexit.attr("register")(internal_m.attr("exit_cleanup"));
}
