#include "novatel_edie/common/logger.hpp"

#include "bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
namespace spd = spdlog;

void init_common_logger(nb::module_& m)
{
    nb::enum_<spdlog::level::level_enum>(m, "LogLevel")
        .value("TRACE", spd::level::trace)
        .value("DEBUG", spd::level::debug)
        .value("INFO", spd::level::info)
        .value("WARN", spd::level::warn)
        .value("ERR", spd::level::err)
        .value("CRITICAL", spd::level::critical)
        .value("OFF", spd::level::off);

    nb::enum_<spdlog::pattern_time_type>(m, "LogPatternTimeType")
        .value("LOCAL", spdlog::pattern_time_type::local)
        .value("UTC", spdlog::pattern_time_type::utc);

    nb::class_<spdlog::logger>(m, "_SpdlogLogger")
        .def(nb::init<std::string>())
        .def(
            "log", [](spd::logger& logger, spd::level::level_enum level, std::string_view msg) { logger.log(level, msg); }, "level"_a, "msg"_a)
        .def(
            "trace", [](spd::logger& logger, std::string_view msg) { logger.trace(msg); }, "msg"_a)
        .def(
            "debug", [](spd::logger& logger, std::string_view msg) { logger.debug(msg); }, "msg"_a)
        .def(
            "info", [](spd::logger& logger, std::string_view msg) { logger.info(msg); }, "msg"_a)
        .def(
            "warn", [](spd::logger& logger, std::string_view msg) { logger.warn(msg); }, "msg"_a)
        .def(
            "error", [](spd::logger& logger, std::string_view msg) { logger.error(msg); }, "msg"_a)
        .def(
            "critical", [](spd::logger& logger, std::string_view msg) { logger.critical(msg); }, "msg"_a)
        .def("should_log", &spd::logger::should_log, "level"_a)
        .def("should_backtrace", &spd::logger::should_backtrace)
        .def("set_level", &spd::logger::set_level, "level"_a)
        .def_prop_ro("level", &spd::logger::level)
        .def_prop_ro("name", &spd::logger::name)
        // .def("set_formatter", &spd::logger::set_formatter, "formatter"_a)
        .def("set_pattern", &spd::logger::set_pattern, "pattern"_a, "time_type"_a = spd::pattern_time_type::local)
        .def("enable_backtrace", &spd::logger::enable_backtrace, "n_messages"_a)
        .def("disable_backtrace", &spd::logger::disable_backtrace)
        .def("dump_backtrace", &spd::logger::dump_backtrace)
        .def("flush", &spd::logger::flush)
        .def("flush_on", &spd::logger::flush_on, "level"_a)
        .def_prop_ro("flush_level", &spd::logger::flush_level)
        .def("set_error_handler", &spd::logger::set_error_handler, "handler"_a)
        .def("clone", &spd::logger::clone, "logger_name"_a);

    nb::class_<Logger>(m, "Logger")
        .def(nb::init<>())
        // .def(nb::init<std::string>(), "logger_config_path_"_a)
        .def_static("shutdown", &Logger::Shutdown, "Stop any running threads started by spdlog and clean registry loggers")
        .def_static("set_logging_level", &Logger::SetLoggingLevel, "level"_a, "Change the global spdlog logging level")
        .def_static("register_logger", &Logger::RegisterLogger, "name"_a, "Register a logger with the root logger's sinks.")
        .def_static("add_console_logging", &Logger::AddConsoleLogging, "logger"_a, "level"_a = spd::level::debug, "Add console output to the logger")
        .def_static("add_rotating_file_logger", &Logger::AddRotatingFileLogger, "logger"_a, "level"_a = spd::level::debug, "file_name"_a = "edie.log",
                    "file_size"_a = 5 * 1024 * 1024, "max_files"_a = 2, "rotate_on_open"_a = true, "Add rotating file output to the logger");
}
