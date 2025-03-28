// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file logger.hpp
// ===============================================================================

#ifndef LOGGER_H
#define LOGGER_H

// Set the default logging level for SPDLOG_XXX macros
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog_setup/spdlog_setup.hpp>

//============================================================================
//! \class LoggerManager
//! \brief Custom logger class.
//============================================================================
class LoggerManager
{
  public:
    //----------------------------------------------------------------------------
    //! \brief Preform any cleanup and destroy the manager object.
    //----------------------------------------------------------------------------
    virtual void Shutdown() = 0;

    //----------------------------------------------------------------------------
    //! \brief Virutal destructor for required for virtual functions.
    //!
    //! In general leaving cleanup to destructor is unsafe as LoggerManager's
    //! lifetime is undefined in relation to other static objects it relies upon.
    //! Instead `Shutdown()` method of concrete subclass should be called prior
    //! to termination.
    //----------------------------------------------------------------------------
    virtual ~LoggerManager() = 0;

    //----------------------------------------------------------------------------
    //! \brief Register a logger with the root logger's sinks.
    //! \param[in] sLoggerName_ A name for the logger
    //! \return A shared pointer to the logger.
    //----------------------------------------------------------------------------
    virtual std::shared_ptr<spdlog::logger> RegisterLogger(const std::string& sLoggerName_) = 0;
};

//============================================================================
//! \class CPPLoggerManager
//! \brief The concrete LoggerManager for use with the C++ API
//============================================================================
class CPPLoggerManager : public LoggerManager
{
  private:
    std::once_flag loggerFlag;
    std::mutex loggerMutex;
    std::shared_ptr<spdlog::logger> rootLogger;
    std::map<std::string, std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>> rotatingFiles;

    //----------------------------------------------------------------------------
    //! \brief Preform a basic initialization of the root logger.
    //----------------------------------------------------------------------------
    void InitRootLogger()
    {
        std::call_once(loggerFlag, [this]() {
            rootLogger = spdlog::stdout_color_mt("root");
            rootLogger->set_level(spdlog::level::info);
            rootLogger->flush_on(spdlog::level::debug);
            set_default_logger(rootLogger);
            rootLogger->info("Logger initialized.");
        });
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief Destructs the CPPLoggerManager.
    //!
    //! Shutdown should be called first.
    //----------------------------------------------------------------------------
    ~CPPLoggerManager() = default;

    //----------------------------------------------------------------------------
    //! \brief Flushes all rotating file sinks and shutdown spdlog.
    //!
    //! Must be called by users before program termination.
    //----------------------------------------------------------------------------
    void Shutdown() override
    {
        std::lock_guard<std::mutex> lock(loggerMutex);
        if (rootLogger) { rootLogger->flush(); }
        for (const auto& [fileName, sink] : rotatingFiles)
        {
            sink->flush();
            rootLogger->info("Removed rotating file sink: {}", fileName);
        }
        rotatingFiles.clear();
        spdlog::shutdown();
    }

    //----------------------------------------------------------------------------
    //! \brief Registers a logger with the root logger's sinks.
    //!
    //! \param[in] sLoggerName_ A name for the logger.
    //! \return A shared pointer to the logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> RegisterLogger(const std::string& sLoggerName_) override
    {
        std::lock_guard lock(loggerMutex);
        InitRootLogger();
        rootLogger->debug("RegisterLogger(\"{}\")", sLoggerName_);
        std::shared_ptr<spdlog::logger> pclLogger;
        try
        {
            pclLogger = spdlog::get(sLoggerName_);

            if (pclLogger == nullptr)
            {
                pclLogger = std::make_shared<spdlog::logger>(sLoggerName_, begin(rootLogger->sinks()), end(rootLogger->sinks()));
                pclLogger->set_level(rootLogger->level());
                register_logger(pclLogger);
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            SPDLOG_ERROR("RegisterLogger(\"{}\") init failed: {}", sLoggerName_, ex.what());
        }

        return pclLogger;
    }

    //----------------------------------------------------------------------------
    //! \brief Changes the global spdlog logging level.
    //!
    //! \param[in] eLevel_ The logging level to set.
    //----------------------------------------------------------------------------
    void SetLoggingLevel(spdlog::level::level_enum eLevel_) { spdlog::set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Initializes the logger with an optional configuration file.
    //!
    //! \param[in] configPath_ The path to the configuration file.
    //----------------------------------------------------------------------------
    void InitLogger(const std::filesystem::path& configPath_ = "")
    {
        if (rootLogger)
        {
            rootLogger->warn("Root logger already initialized. Configuration from '{}' ignored.", configPath_.string());
            return;
        }

        try
        {
            if (configPath_.empty()) { InitRootLogger(); }
            else
            {
                spdlog_setup::from_file(configPath_.string());
                InitRootLogger();
                rootLogger->info("Initialized with file: {}", configPath_.string());
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Logger initialization failed: " << ex.what() << '\n';
        }
    }

    //----------------------------------------------------------------------------
    //! \brief Adds console output to a logger.
    //!
    //! \param[in] lgr  The logger to add a console sink to.
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void AddConsoleLogging(const std::shared_ptr<spdlog::logger>& lgr, spdlog::level::level_enum eLevel_ = spdlog::level::info)
    {
        std::lock_guard<std::mutex> lock(loggerMutex);
        auto pclConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        pclConsoleSink->set_level(eLevel_);
        pclConsoleSink->set_pattern("%v");
        lgr->sinks().emplace_back(pclConsoleSink);
    }

    //----------------------------------------------------------------------------
    //! \brief Adds file output to a logger.
    //!
    //! \param[in] lgr  The logger to add a rotating file sink to.
    //! \param[in] eLevel_  Logging level to enable.
    //! \param[in] sFileName_  Logger output file name.
    //! \param[in] uiFileSize_  Max file size.
    //! \param[in] uiMaxFiles_  Max number of rotating files.
    //! \param[in] bRotateOnOpen_  Rotate files on open.
    //----------------------------------------------------------------------------
    void AddRotatingFileLogger(const std::shared_ptr<spdlog::logger>& lgr, spdlog::level::level_enum level = spdlog::level::info,
                               const std::string& sFileName = "default.log", size_t maxFileSize = 5 * 1024 * 1024, size_t maxFiles = 3,
                               bool rotateOnOpen = true)
    {
        std::lock_guard<std::mutex> lock(loggerMutex);
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(sFileName, maxFileSize, maxFiles, rotateOnOpen);
        sink->set_level(level);
        sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] %v");
        lgr->sinks().emplace_back(sink);
        rotatingFiles[sFileName] = sink;
        rootLogger->info("Added rotating file sink: {}", sFileName);
    }
};

// Internal access point for logging management
extern std::unique_ptr<LoggerManager> pclLoggerManager;

//----------------------------------------------------------------------------
//! \brief Gets a LoggerManager to alter logging configuration.
//!
//! \return A pointer to the current LoggingManager.
//----------------------------------------------------------------------------
extern CPPLoggerManager* GetLoggerManager();

// Macro for easy user creation of loggers
#define LOGGER_MANAGER GetLoggerManager()
#define CREATE_LOGGER() GetLoggerManager()->RegisterLogger(std::filesystem::path(__FILE__).stem().string())

//----------------------------------------------------------------------------
//! \brief A wrapper class for CPPLoggerManager to allow backwards compatibility in the API
//----------------------------------------------------------------------------
class Logger
{
  public:
    static void InitLogger(const std::filesystem::path& configPath = "") { GetLoggerManager()->InitLogger(configPath); }
    static void Shutdown() { GetLoggerManager()->Shutdown(); }
    static void SetLoggingLevel(spdlog::level::level_enum eLevel_) { GetLoggerManager()->SetLoggingLevel(eLevel_); }
    static std::shared_ptr<spdlog::logger> RegisterLogger(const std::string& sLoggerName_)
    {
        return GetLoggerManager()->RegisterLogger(sLoggerName_);
    }
    static void AddConsoleLogging(const std::shared_ptr<spdlog::logger>& lgr, spdlog::level::level_enum eLevel_ = spdlog::level::info)
    {
        GetLoggerManager()->AddConsoleLogging(lgr, eLevel_);
    }
    static void AddRotatingFileLogger(const std::shared_ptr<spdlog::logger>& lgr, spdlog::level::level_enum level = spdlog::level::info,
                                      const std::string& sFileName = "default.log", size_t maxFileSize = 5 * 1024 * 1024, size_t maxFiles = 3,
                                      bool rotateOnOpen = true)
    {
        GetLoggerManager()->AddRotatingFileLogger(lgr, level, sFileName, maxFiles, maxFileSize, rotateOnOpen);
    }
};

#endif // LOGGER_H
