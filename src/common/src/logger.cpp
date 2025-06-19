#include "novatel_edie/common/logger.hpp"

LoggerManager::~LoggerManager() = default;

static LoggerManager* GetSetUniquePointer(LoggerManager* pclLoggerManager_)
{
    static std::unique_ptr<LoggerManager> pclLoggerManager = std::make_unique<CPPLoggerManager>();
    if (pclLoggerManager_ != nullptr) { pclLoggerManager.reset(pclLoggerManager_); }
    return pclLoggerManager.get();
}

LoggerManager* GetBaseLoggerManager() { return GetSetUniquePointer(nullptr); }

CPPLoggerManager* GetLoggerManager() { return dynamic_cast<CPPLoggerManager*>(GetBaseLoggerManager()); }

void SetLoggerManager(LoggerManager* manager) { GetSetUniquePointer(manager); }
