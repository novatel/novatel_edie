#include "novatel_edie/common/logger.hpp"


LoggerManager::~LoggerManager() {};

std::unique_ptr<LoggerManager> pclLoggerManager = std::make_unique<CPPLoggerManager>();

CPPLoggerManager* GetLoggerManager() {
    CPPLoggerManager* pclManager = dynamic_cast<CPPLoggerManager*>(pclLoggerManager.get());
    return pclManager;
}
