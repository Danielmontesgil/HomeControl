#pragma once
#include "ILogger.h"

class ConsoleLogger : public ILogger {
public:
    void info(const std::string& tag, const std::string& message) override;
    void debug(const std::string& tag, const std::string& message) override;
    void warn(const std::string& tag, const std::string& message) override;
    void error(const std::string& tag, const std::string& message) override;
};
