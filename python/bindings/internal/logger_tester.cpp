#include "novatel_edie/common/logger.hpp"
#include "../py_logger.hpp"

namespace nb = nanobind;
using namespace nb::literals;

class PyLoggerTester
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("logger_tester")};

  public:
    void SwitchLogger(std::string name) { pclMyLogger = GetBaseLoggerManager()->RegisterLogger(name); }
    void LogDebug(std::string message) { SPDLOG_LOGGER_DEBUG(pclMyLogger, message); }
    void LogInfo(std::string message) { SPDLOG_LOGGER_INFO(pclMyLogger, message); }
    void LogWarn(std::string message) { SPDLOG_LOGGER_WARN(pclMyLogger, message); }
    void LogError(std::string message) { SPDLOG_LOGGER_ERROR(pclMyLogger, message); }
    void LogCritical(std::string message) { SPDLOG_LOGGER_CRITICAL(pclMyLogger, message); }
};

void init_logger_tester(nb::module_& m)
{
    nb::class_<PyLoggerTester>(m, "LoggerTester")
        .def(nb::init<>())
        .def("SetLoggerName", &PyLoggerTester::SwitchLogger)
        .def("LogDebug", &PyLoggerTester::LogDebug)
        .def("LogInfo", &PyLoggerTester::LogInfo)
        .def("LogWarn", &PyLoggerTester::LogWarn)
        .def("LogError", &PyLoggerTester::LogError)
        .def("LogCritical", &PyLoggerTester::LogCritical);
}
