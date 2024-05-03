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
// ! \file common_logger.cpp
// ===============================================================================

#include <string>

#include "common_logger.hpp"
#include "decoders/common/api/logger.hpp"

void CommonLoggerSetup() { Logger::InitLogger(); }

void CommonLoggerSetupFromFile(char* pcLoggerConfigPath_) { Logger::InitLogger(std::string(pcLoggerConfigPath_)); }

bool CommonLoggerSetLoggerLevel(int32_t iLogLevel_)
{
    return iLogLevel_ >= spdlog::level::level_enum::trace && iLogLevel_ < spdlog::level::level_enum::n_levels
           ? spdlog::set_level(static_cast<spdlog::level::level_enum>(iLogLevel_)),
           true : false;
}

void CommonLoggerShutdownLogger() { Logger::Shutdown(); }

bool CommonLoggerLog(int32_t iLogLevel_, char* pucMessage_)
{
    std::shared_ptr<spdlog::logger> pclLogger = spdlog::get("logger_");

    if (!pclLogger) { pclLogger = Logger::RegisterLogger("decoder_dll"); }

    if (iLogLevel_ >= spdlog::level::level_enum::trace && iLogLevel_ < spdlog::level::level_enum::n_levels)
    {
        switch (static_cast<spdlog::level::level_enum>(iLogLevel_))
        {
        case SPDLOG_LEVEL_TRACE: pclLogger->trace(pucMessage_); break;
        case SPDLOG_LEVEL_DEBUG: pclLogger->debug(pucMessage_); break;
        case SPDLOG_LEVEL_INFO: pclLogger->info(pucMessage_); break;
        case SPDLOG_LEVEL_WARN: pclLogger->warn(pucMessage_); break;
        case SPDLOG_LEVEL_ERROR: pclLogger->error(pucMessage_); break;
        case SPDLOG_LEVEL_CRITICAL: pclLogger->critical(pucMessage_); break;
        case SPDLOG_LEVEL_OFF: break; // Do no logging
        default: SPDLOG_LOGGER_ERROR(pclLogger, "Unknown log level {}", iLogLevel_); break;
        }
        return true;
    }
    return false;
}
