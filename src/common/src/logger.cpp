#include "novatel_edie/common/logger.hpp"

LoggerManager::~LoggerManager() = default;

std::unique_ptr<LoggerManager> pclLoggerManager = std::make_unique<CPPLoggerManager>();

CPPLoggerManager* GetLoggerManager()
{
    return dynamic_cast<CPPLoggerManager*>(pclLoggerManager.get());
}
