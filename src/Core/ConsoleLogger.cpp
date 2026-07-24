#include "Core/ConsoleLogger.h"
#include <iostream>

void ConsoleLogger::info(const std::string& tag, const std::string& message) {
    std::cout << "[INFO][" << tag << "] " << message << std::endl;
}

void ConsoleLogger::debug(const std::string& tag, const std::string& message) {
    std::cout << "[DEBUG][" << tag << "] " << message << std::endl;
}

void ConsoleLogger::warn(const std::string& tag, const std::string& message) {
    std::cout << "[WARN][" << tag << "] " << message << std::endl;
}

void ConsoleLogger::error(const std::string& tag, const std::string& message) {
    std::cerr << "[ERROR][" << tag << "] " << message << std::endl;
}
