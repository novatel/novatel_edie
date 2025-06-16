#include "novatel_edie/common/logger.hpp"

std::unique_ptr<LoggerManager> pclLoggerManager = std::make_unique<CPPLoggerManager>();

LoggerManager::~LoggerManager() = default;

CPPLoggerManager* GetLoggerManager() { return dynamic_cast<CPPLoggerManager*>(pclLoggerManager.get()); }

LoggerManager* GetBaseLoggerManager()
{
    if (pclLoggerManager == nullptr) { throw std::runtime_error("GetBaseLoggerManager(): LoggerManager is not initialized"); }
    return pclLoggerManager.get();
}

void SetLoggerManager(LoggerManager* manager)
{
    if (manager == nullptr) { throw std::runtime_error("SetLoggerManager(): Cannot set a null LoggerManager"); }
    pclLoggerManager.reset(manager);
}
