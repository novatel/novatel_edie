#pragma once

#include <iostream>
#include <memory>
#include <mutex>

#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include "bindings_core.hpp"
#include "novatel_edie/common/logger.hpp"

namespace nb = nanobind;

namespace python_log {

//============================================================================
//! \class python_sink
//! \brief A spdlog sink that marshalls logs into a python logger.
//============================================================================
class python_sink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
{
  private:
    nb::handle py_logger;

    //----------------------------------------------------------------------------
    // ! \brief Checks if a python logger is enabled for a specified log level.
    // ! \param[in] py_logger The python logger to check for.
    // ! \param[in] level The python log level to check for.
    // ! \return Whether the logger is enabled for the level.
    //----------------------------------------------------------------------------
    inline bool is_enabled(int level) { return nb::cast<bool>(py_logger.attr("isEnabledFor")(level)); }

    //----------------------------------------------------------------------------
    // ! \brief Convert from an spdlog level enum to an appropriate integer value.
    // ! \param[in] level The spdlog level enum value.
    // ! \return The integer representing that same log level in python.
    //----------------------------------------------------------------------------
    inline int get_python_log_level(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace: return 5;     // SUB-DEBUG
        case spdlog::level::debug: return 10;    // DEBUG
        case spdlog::level::info: return 20;     // INFO
        case spdlog::level::warn: return 30;     // WARNING
        case spdlog::level::err: return 40;      // ERROR
        case spdlog::level::critical: return 50; // CRITICAL
        default: throw std::runtime_error("Logging was performed at an unknown level");
        }
    }

  public:
    //----------------------------------------------------------------------------
    // ! \brief Create a python_sink to a specfic Python logger.
    // ! \param[in] py_logger_ A handle to the Python logger object.
    //----------------------------------------------------------------------------
    explicit python_sink(nb::handle py_logger_) : py_logger(py_logger_) {}

    //----------------------------------------------------------------------------
    // ! \brief Send a log message to the attached python logger if enabled.
    // ! \param[in] msg The message to send to the python logger.
    //----------------------------------------------------------------------------
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        int level = get_python_log_level(msg.level);
        if (is_enabled(level))
        {
            nb::object source_filename = msg.source.filename ? nb::str(msg.source.filename) : nb::none();
            nb::object source_funcname = msg.source.funcname ? nb::str(msg.source.funcname) : nb::none();
            nb::str msg_payload = nb::str(msg.payload.data(), msg.payload.size());
            // Formatting arguments and exectution info are None
            nb::object record = py_logger.attr("makeRecord")(py_logger.attr("name"), level, source_filename, msg.source.line, msg_payload, nb::none(),
                                                             nb::none(), source_funcname);
            py_logger.attr("handle")(record);
        }
    }

    //----------------------------------------------------------------------------
    // ! \brief Preserve default flush behavior.
    //----------------------------------------------------------------------------
    void flush_() override {}
};

//============================================================================
//! \class PyLoggerManager
//! \brief A LoggerManager capable of registering connections to Python loggers.
//!
//! Will automatically be set as the global `pclLoggerManager` during
//! Python API initialization.
//============================================================================
class PyLoggerManager : public LoggerManager
{
  private:
    std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;

    //----------------------------------------------------------------------------
    // ! \brief Convert from the provided C++ logger name to an appropriate Python one.
    // ! \return The name for the logger on the Python side.
    //----------------------------------------------------------------------------
    std::string CLoggerNameToPyLoggerName(std::string logger_name_)
    {
        std::string py_name = "novatel_edie";
        if (logger_name_ == "header_decoder" || logger_name_ == "message_decoder") { py_name += ".decoder"; }
        return py_name + "." + logger_name_;
    }

  public:
    //----------------------------------------------------------------------------
    // ! \brief Flush all python logger sinks and clear out logger references.
    // !
    // ! spdlog::shutdown is not called due to certain static variables 
    // ! required to do so being possibly deallocated at destruction time.
    // ! This is only acceptable because the PyLoggerManager does not make use 
    // ! of asynchronous logging and does use the spdlog logger registry.
    //----------------------------------------------------------------------------
    ~PyLoggerManager() override
    {
        for (const auto& [fileName, sink] : loggers) { sink->flush(); }
        loggers.clear();
    }

    //----------------------------------------------------------------------------
    // ! \brief Register a new logger with a python logger sink.
    // ! \return A shared pointer to the newly registered logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> RegisterLogger(const std::string& logger_name_)
    {
        // Get logger if it exists
        std::string py_name = CLoggerNameToPyLoggerName(logger_name_);
        auto it = loggers.find(py_name);
        if (it != loggers.end()) { return it->second; }
        // Create logger
        nb::object py_logger = nb::module_::import_("logging").attr("getLogger")(py_name);
        std::shared_ptr<python_sink> sink = std::make_shared<python_sink>(py_logger);
        std::shared_ptr<spdlog::logger> spd_logger = std::make_shared<spdlog::logger>(py_name, sink);
        // Ensure all messages are sent to python_sink
        spd_logger->set_level(spdlog::level::trace);
        // Save and return logger - No need to use spdlog registry as accessibility is only through python
        loggers[py_name] = spd_logger;
        return spd_logger;
    }
};

} // namespace python_log
