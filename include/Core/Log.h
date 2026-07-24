#pragma once
#include "ILogger.h"
#include <memory>

class Log {
public:
    static void setLogger(std::shared_ptr<ILogger> logger);
    static void setLogMask(unsigned int mask);
    static unsigned int getLogMask();

    static void info(const std::string& tag, const std::string& message);
    static void debug(const std::string& tag, const std::string& message);
    static void warn(const std::string& tag, const std::string& message);
    static void error(const std::string& tag, const std::string& message);

private:
    static std::shared_ptr<ILogger> m_logger;
    static unsigned int m_logMask;
};
