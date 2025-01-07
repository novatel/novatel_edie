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

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog_setup/spdlog_setup.hpp>

//============================================================================
//! \class Logger
//! \brief Custom logger class.
//============================================================================
class Logger
{
  private:
    inline static std::once_flag loggerFlag;
    inline static std::mutex loggerMutex;
    inline static std::shared_ptr<spdlog::logger> rootLogger;
    inline static std::map<std::string, std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>> rotatingFiles;

    static void InitLoggerHelper()
    {
        std::call_once(loggerFlag, []() {
            rootLogger = spdlog::stdout_color_mt("root");
            rootLogger->set_level(spdlog::level::info);
            rootLogger->flush_on(spdlog::level::debug);
            set_default_logger(rootLogger);
            rootLogger->info("Logger initialized.");
        });
    }

  public:
    static void InitLogger(const std::filesystem::path& configPath)
    {
        if (rootLogger)
        {
            rootLogger->warn("Root logger already initialized. Configuration from '{}' ignored.", configPath.string());
            return;
        }

        try
        {
            if (configPath.empty()) { InitLoggerHelper(); }
            else
            {
                spdlog_setup::from_file(configPath.string());
                InitLoggerHelper();
                rootLogger->info("Initialized with file: {}", configPath.string());
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Logger initialization failed: " << ex.what() << '\n';
        }
    }

    /*! \brief Stop any running threads started by spdlog and clean registry loggers.
     */
    static void Shutdown()
    {
        if (rootLogger) { rootLogger->flush(); }
        spdlog::shutdown();
    }

    /*! \brief Change the global spdlog logging level
     */
    static void SetLoggingLevel(spdlog::level::level_enum eLevel_) { spdlog::set_level(eLevel_); }

    /*! \brief Register a logger with the root logger's sinks.
     *
     *  \param sLoggerName_ a unique name for the logger
     *  \return std::shared_ptr<spdlog::logger>
     */
    static std::shared_ptr<spdlog::logger> RegisterLogger(std::string sLoggerName_)
    {
        if (!rootLogger) { InitLoggerHelper(); }
        std::lock_guard lock(loggerMutex);
        rootLogger->debug("Logger::RegisterLogger(\"{}\")", sLoggerName_);
        std::shared_ptr<spdlog::logger> pclLogger;
        try
        {
            pclLogger = spdlog::get(sLoggerName_);

            if (pclLogger == nullptr)
            {
                // Get the root logger sinks
                std::vector<spdlog::sink_ptr> vRootSinks = rootLogger->sinks();
                pclLogger = std::make_shared<spdlog::logger>(sLoggerName_, begin(vRootSinks), end(vRootSinks));
                // Inherit the root logger level by default
                pclLogger->set_level(rootLogger->level());
                register_logger(pclLogger);
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            // TODO: why does deleting this line break the pipeline?
            std::cout << (spdlog::get(sLoggerName_) == nullptr ? "null" : "not null") << '\n';
            std::cout << "Logger::RegisterLogger() init failed: " << ex.what() << '\n';
            SPDLOG_ERROR("Logger::RegisterLogger(\"{}\") init failed: {}", sLoggerName_, ex.what());
        }

        // return a shared_ptr that may be assigned where the usage_count is incremented
        return pclLogger;
    }

    /** \brief Add console output to the logger
     *  \param[in] eLevel_  The logging level to enable.
     */
    static void AddConsoleLogging(const std::shared_ptr<spdlog::logger>& lgr, spdlog::level::level_enum eLevel_ = spdlog::level::debug)
    {
        // Console sink, with no formatting/metadata
        auto pclConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        pclConsoleSink->set_level(eLevel_);
        pclConsoleSink->set_pattern("%v");

        lgr->sinks().push_back(pclConsoleSink);
    }

    /** \brief Add file output to the logger
     *  \param[in] eLevel_  Logging level to enable.
     *  \param[in] sFileName_  Logger output file name.
     *  \param[in] uiFileSize_  Max file size.
     *  \param[in] uiMaxFiles_  Max number of rotating files.
     *  \param[in] bRotateOnOpen_  Rotate files on open.
     */
    static void AddRotatingFileLogger(const std::shared_ptr<spdlog::logger>& lgr, spdlog::level::level_enum level = spdlog::level::info,
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

#endif // LOGGER_H
