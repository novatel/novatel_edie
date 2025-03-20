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

//============================================================================
//! \class python_sink
//! \brief A spdlog sink that marshalls logs into a python logger.
//============================================================================
class python_sink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
{
  private:
    // A reference to the python logger which messages will be pushed to
    nb::handle py_logger;

    //----------------------------------------------------------------------------
    //! \brief Checks if a python logger is enabled for a specified log level.
    //! \param[in] py_logger The python logger to check for.
    //! \param[in] level The python log level to check for.
    //! \return Whether the logger is enabled for the level.
    //----------------------------------------------------------------------------
    inline bool is_enabled(int level) { return nb::cast<bool>(py_logger.attr("isEnabledFor")(level)); }

    //----------------------------------------------------------------------------
    //! \brief Converts from an spdlog level enum to an appropriate integer value.
    //! \param[in] level The spdlog level enum value.
    //! \return The integer representing that same log level in python.
    //----------------------------------------------------------------------------
    inline int get_python_log_level(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace: return 1;     // SUB-DEBUG
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
    //! \brief Creates a python_sink to a specfic Python logger.
    //! \param[in] py_logger_ A handle to the Python logger object.
    //----------------------------------------------------------------------------
    explicit python_sink(nb::handle py_logger_) : py_logger(py_logger_) {}

    //----------------------------------------------------------------------------
    //! \brief Sends a log message to the attached python logger if enabled.
    //! \param[in] msg The message to send to the python logger.
    //----------------------------------------------------------------------------
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        int level = get_python_log_level(msg.level);
        if (is_enabled(level))
        {
            nb::object source_filename = msg.source.filename ? nb::str(msg.source.filename) : nb::none();
            nb::object source_funcname = msg.source.funcname ? nb::str(msg.source.funcname) : nb::none();
            nb::str msg_payload = nb::str(msg.payload.data(), msg.payload.size());
            // Formatting arguments and execution info are None
            nb::object record = py_logger.attr("makeRecord")(py_logger.attr("name"), level, source_filename, msg.source.line, msg_payload, nb::none(),
                                                             nb::none(), source_funcname);
            py_logger.attr("handle")(record);
        }
    }

    //----------------------------------------------------------------------------
    //! \brief Preserves default flush behavior.
    //----------------------------------------------------------------------------
    void flush_() override {}
};

//============================================================================
//! \class PyLoggerManager
//! \brief A LoggerManager capable of registering connections to Python loggers.
//!
//! Manages all spd loggers associated with python ones and is responsible for
//! updating their log level as appropriate.
//!
//! Will automatically be set as the global `pclLoggerManager` during
//! Python API initialization.
//!
//! Only designed for single-threaded access.
//============================================================================
class PyLoggerManager : public LoggerManager
{
  private:
    // whether any messages should be logged
    bool disabled = false;
    // the set of spd loggers under management
    std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;
    // functions used for setting log level on Python side
    std::map<std::string, nb::callable> set_level_funcs;
    // a reference to the set of novatel_edie internal python functions - stored as handle to avoid cleanup
    nb::handle internal_mod = nb::none();

    //----------------------------------------------------------------------------
    //! \brief Converts from the provided C++ logger name to an appropriate Python one.
    //!
    //! \return The name for the logger on the Python side.
    //----------------------------------------------------------------------------
    std::string CLoggerNameToPyLoggerName(std::string logger_name_)
    {
        std::string py_name = "novatel_edie";
        if (logger_name_ == "header_decoder" || logger_name_ == "message_decoder") { py_name += ".decoder"; }
        return py_name + "." + logger_name_;
    }

    //----------------------------------------------------------------------------
    //! \brief Converts from a Python log level to and spdlog one.
    //!
    //! \return The appropriate spdlog level.
    //----------------------------------------------------------------------------
    inline spdlog::level::level_enum get_spd_log_level(int level)
    {
        if (level < 10) { return spdlog::level::trace; }
        else if (level < 20) { return spdlog::level::debug; }
        else if (level < 30) { return spdlog::level::info; }
        else if (level < 40) { return spdlog::level::warn; }
        else if (level < 50) { return spdlog::level::err; }
        else if (level == 50) { return spdlog::level::critical; }
        else { return spdlog::level::off; }
    }

    //----------------------------------------------------------------------------
    //! \brief Gets the effective level of a logger as an spdlog level.
    //!
    //! \param[in] logger_ The logger whose level to get.
    //! \return The appropriate spdlog level.
    //----------------------------------------------------------------------------
    spdlog::level::level_enum GetEffectiveLogLevel(nb::handle logger_)
    {
        return get_spd_log_level(nb::cast<int>(logger_.attr("getEffectiveLevel")()));
    }

    //----------------------------------------------------------------------------
    //! \brief Injects a hook into the setLevel function of the provided logger.
    //!
    //! Allows the PyLoggerManager to update an spdlogger's level on changes to the
    //! corresponding Python logger's level.
    //!
    //! \param[in] logger_ The logger whose level to get.
    //! \return The appropriate spdlog level.
    //----------------------------------------------------------------------------
    void InjectHook(nb::handle logger)
    {
        std::string logger_name = nb::cast<std::string>(logger.attr("name"));
        // If hook is already injected return
        if (set_level_funcs.find(logger_name) != set_level_funcs.end()) { return; }
        // Otherwise save old method and inject hook into object
        nb::callable original_func = logger.attr("setLevel");
        set_level_funcs[logger_name] = original_func;
        nb::handle set_level_func = internal_mod.attr("set_level");
        nb::module_ types = nb::module_::import_("types");
        nb::object instance_set_level_func = types.attr("MethodType")(set_level_func, logger);
        logger.attr("setLevel") = instance_set_level_func;
    }

    //----------------------------------------------------------------------------
    //! \brief Registers a new logger associated with a Python one.
    //!
    //! \param[in] logger_name_  The name of the logger on the Python side.
    //! \return A shared pointer to the newly registered or pre-existing spd logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> RegisterPythonLogger(const std::string& logger_name_)
    {
        // Get the logger if it exists
        std::cout << "Registering " << logger_name_ << std::endl;
        auto it = loggers.find(logger_name_);
        if (it != loggers.end()) { return it->second; }

        // Register its parent logger first
        if (logger_name_ != "root")
        {
            std::string parent_name;
            size_t last_delimiter = logger_name_.find_last_of('.');
            if (last_delimiter != std::string::npos) { parent_name = logger_name_.substr(0, last_delimiter); }
            else { parent_name = "root"; }
            RegisterPythonLogger(parent_name);
        }

        // Create logger
        nb::handle py_logger = nb::module_::import_("logging").attr("getLogger")(logger_name_);
        std::shared_ptr<python_sink> sink = std::make_shared<python_sink>(py_logger);
        std::shared_ptr<spdlog::logger> spd_logger = std::make_shared<spdlog::logger>(logger_name_, sink);

        // Set appropriate log level and add callback into the logger object to update on change
        if (disabled) { spd_logger->set_level(spdlog::level::off); }
        else { spd_logger->set_level(GetEffectiveLogLevel(py_logger)); }
        InjectHook(py_logger);

        // Save and return logger
        loggers[logger_name_] = spd_logger;
        return spd_logger;
    }

    //----------------------------------------------------------------------------
    //! \brief Refreshes the spdlog
    //!
    //! \param[in] logger_name_  The name of the logger on the Python side.
    //! \return A shared pointer to the newly registered or pre-existing spd logger.
    //----------------------------------------------------------------------------
    void RefreshSpdLoggerLevel(nb::handle logger)
    {
        std::string logger_name = nb::cast<std::string>(logger.attr("name"));

        // Check whether logger is under management
        auto it = loggers.find(logger_name);
        if (it == loggers.end()) { return; }

        // Refresh any children also under management
        nb::set children = nb::cast<nb::set>(logger.attr("getChildren")());
        for (auto child : children) { RefreshSpdLoggerLevel(child); }

        // Refresh logger level
        std::shared_ptr<spdlog::logger> spd_logger = it->second;
        spd_logger->set_level(GetEffectiveLogLevel(logger));
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief Cleans up managed loggers and stored Python objects.
    //!
    //! Flushes managed spd loggers, deallocates them, and deallocates
    //! stored Python setLevel callbacks.
    //----------------------------------------------------------------------------
    void Shutdown() override
    {
        for (const auto& [name, logger] : loggers) { logger->flush(); }
        loggers.clear();
        set_level_funcs.clear();
    }

    void SetLoggerLevel(nb::handle logger, nb::args args_, nb::kwargs kwargs_)
    {
        // Call original function first in case of exception
        std::string logger_name = nb::cast<std::string>(logger.attr("name"));
        auto it = set_level_funcs.find(logger_name);
        if (it == set_level_funcs.end()) { throw std::runtime_error("The logger is under management but has no associated setLevel function"); }
        nb::callable original_func = it->second;
        try
        {
            // Directly forward arguments to original setLevel function
            original_func(*args_, **kwargs_);
        }
        catch (const nb::python_error& e)
        {
            // Throw errors according to original function implementation
            throw;
        }

        // Update spdlog levels to match new ones
        if (!disabled) { RefreshSpdLoggerLevel(logger); }
    }

    //----------------------------------------------------------------------------
    //! \brief Sets the reference for accessing internal novatel_edie Python functions.
    //!
    //! Includes `set_level` function used for triggering custom log level manipulation.
    //!
    //! \param[in] internal_mod_ The module of internal functionality.
    //----------------------------------------------------------------------------
    void SetInternalMod(nb::module_ internal_mod_) { internal_mod = internal_mod_; }

    //----------------------------------------------------------------------------
    //! \brief Registers a new logger associated with a python one.
    //!
    //! \param[in] logger_name_  The name of the logger on the C++ side.
    //! \return A shared pointer to the newly registered or pre-existing logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> RegisterLogger(const std::string& logger_name_)
    {
        // Get logger if it exists
        std::string py_name = CLoggerNameToPyLoggerName(logger_name_);
        return RegisterPythonLogger(py_name);
    }

    //----------------------------------------------------------------------------
    //! \brief Disables all logging within C++ code.
    //!
    //! Each spdlog logger under management is set to off.
    //----------------------------------------------------------------------------
    void DisableInternalLogging()
    {
        disabled = true;
        for (const auto& [name, logger] : loggers) { logger->set_level(spdlog::level::off); }
    }

    //----------------------------------------------------------------------------
    //! \brief Enables logging within C++ code.
    //!
    //! Each spdlog logger under management is set the match with python log level.
    //----------------------------------------------------------------------------
    void EnableInternalLogging()
    {
        disabled = false;
        nb::handle logging = nb::module_::import_("logging");
        for (const auto& [name, logger] : loggers)
        {
            nb::handle py_logger = logging.attr("getLogger")(name);
            logger->set_level(GetEffectiveLogLevel(py_logger));
        }
    }
};
