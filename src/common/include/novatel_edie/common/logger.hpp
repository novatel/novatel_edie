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

#ifndef NOVATEL_EDIE_COMMON_LOGGER_H
#define NOVATEL_EDIE_COMMON_LOGGER_H

// Set the default logging level for SPDLOG_XXX macros
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <iostream>
#include <map>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog_setup/spdlog_setup.hpp>

// TODO: this class is mostly obsolete now, would be best to find a way to make the functions standalone
class Logger
{
  private:
    inline static std::mutex mLoggerMutex;
    inline static std::shared_ptr<spdlog::logger> pclMyRootLogger;
    inline static std::map<std::string, std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>> mRotatingFiles;

  public:
    static void InitLogger()
    {
        std::lock_guard<std::mutex> lock(mLoggerMutex);
        try
        {
            pclMyRootLogger = spdlog::get("root");
            if (!pclMyRootLogger)
            {
                pclMyRootLogger = std::make_shared<spdlog::logger>("root");
                pclMyRootLogger->set_level(spdlog::level::info);
                spdlog::register_logger(pclMyRootLogger);

                spdlog::set_default_logger(pclMyRootLogger);
                pclMyRootLogger->flush_on(spdlog::level::debug);
                pclMyRootLogger->debug("Default Logger initialized");
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Logger spdlog init failed: " << ex.what() << std::endl;
        }
        catch (const spdlog_setup::setup_error& ex)
        {
            std::cout << "Logger spdlog_setup failed: " << ex.what() << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Logger failed: " << ex.what() << std::endl;
        }
    }

    static void InitLogger(std::string sLoggerConfigPath_)
    {
        std::lock_guard<std::mutex> lock(mLoggerMutex);
        try
        {
            pclMyRootLogger = spdlog::get("root");
            if (pclMyRootLogger) { SPDLOG_ERROR("Cannot configure logger from file, root logger already exists"); }
            else
            {
                spdlog_setup::from_file(sLoggerConfigPath_);
                pclMyRootLogger = spdlog::get("root");

                spdlog::set_default_logger(pclMyRootLogger);
                pclMyRootLogger->flush_on(spdlog::level::debug);
                pclMyRootLogger->debug("Logger initialized from file: {}", sLoggerConfigPath_);
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << "Logger spdlog init failed: " << ex.what() << std::endl;
        }
        catch (const spdlog_setup::setup_error& ex)
        {
            std::cout << "Logger spdlog_setup failed: " << ex.what() << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cout << "Logger failed: " << ex.what() << std::endl;
        }
    }

    /*! \brief Stop any running threads started by spdlog and clean registry loggers
     */
    static void Shutdown()
    {
        if (pclMyRootLogger) pclMyRootLogger->flush();
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
        if (!pclMyRootLogger) { InitLogger(); }
        std::lock_guard<std::mutex> lock(mLoggerMutex);
        pclMyRootLogger->debug("Logger::RegisterLogger(\"{}\")", sLoggerName_);
        std::shared_ptr<spdlog::logger> pclLogger;
        try
        {
            pclLogger = spdlog::get(sLoggerName_);

            if (pclLogger == nullptr)
            {
                // Get the root logger sinks
                std::vector<spdlog::sink_ptr> vRootSinks = pclMyRootLogger->sinks();
                pclLogger = std::make_shared<spdlog::logger>(sLoggerName_, begin(vRootSinks), end(vRootSinks));
                // Inherit the root logger level by default
                pclLogger->set_level(pclMyRootLogger->level());
                spdlog::register_logger(pclLogger);
            }
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            // TODO: why does deleting this line break the pipeline?
            std::cout << (spdlog::get(sLoggerName_) == nullptr ? "null" : "not null") << std::endl;
            std::cout << "Logger::RegisterLogger() init failed: " << ex.what() << std::endl;
            SPDLOG_ERROR("Logger::RegisterLogger(\"{}\") init failed: {}", sLoggerName_, ex.what());
        }

        // return a shared_ptr that may be assigned where the usage_count is incremented
        return pclLogger;
    }

    /** \brief Add console output to the logger
     *  \param [in] eLevel_  The logging level to enable.
     */
    static void AddConsoleLogging(std::shared_ptr<spdlog::logger> lgr, spdlog::level::level_enum eLevel_ = spdlog::level::debug)
    {
        // Console sink, with no formatting/metadata
        auto pclConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        pclConsoleSink->set_level(eLevel_);
        pclConsoleSink->set_pattern("%v");

        lgr->sinks().push_back(pclConsoleSink);
    }

    /** \brief Add file output to the logger
     *  \param [in] eLevel_  Logging level to enable.
     *  \param [in] sFileName_  Logger output file name.
     *  \param [in] uiFileSize_  Max file size.
     *  \param [in] uiMaxFiles_  Max number of rotating files.
     *  \param [in] bRotateOnOpen_  Rotate files on open.
     */
    static void AddRotatingFileLogger(std::shared_ptr<spdlog::logger> lgr, spdlog::level::level_enum eLevel_ = spdlog::level::debug,
                                      std::string sFileName_ = "edie.log", uint32_t uiFileSize_ = 5 * 1024 * 1024, uint32_t uiMaxFiles_ = 2,
                                      bool bRotateOnOpen_ = true)
    {
        if (mRotatingFiles.count(sFileName_)) { lgr->sinks().push_back(mRotatingFiles.at(sFileName_)); }
        else
        {
            // Rotating file sink, with default formatting/metadata, max 3 files (2 previous + 1 current) of 5MB each
            auto pclFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(sFileName_, uiFileSize_, uiMaxFiles_, bRotateOnOpen_);
            pclFileSink->set_level(eLevel_);
            lgr->sinks().push_back(pclFileSink);
            mRotatingFiles[sFileName_] = pclFileSink;
        }
    }
};

#endif // NOVATEL_EDIE_COMMON_LOGGER_H
