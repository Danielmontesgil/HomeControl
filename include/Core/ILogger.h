#pragma once
#include <string>

enum LogLevel {
    LogLevelNone  = 0,
    LogLevelInfo  = 1 << 0,  // 1 (0001)
    LogLevelDebug = 1 << 1,  // 2 (0010)
    LogLevelWarn  = 1 << 2,  // 4 (0100)
    LogLevelError = 1 << 3,  // 8 (1000)
    LogLevelAll   = 0x0F     // 15 (1111)
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& tag, const std::string& message) = 0;
    virtual void debug(const std::string& tag, const std::string& message) = 0;
    virtual void warn(const std::string& tag, const std::string& message) = 0;
    virtual void error(const std::string& tag, const std::string& message) = 0;
};
