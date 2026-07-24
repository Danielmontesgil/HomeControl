#ifdef __ANDROID__
#include "Core/AndroidLogger.h"
#include <android/log.h>

void AndroidLogger::info(const std::string& tag, const std::string& message) {
    __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", message.c_str());
}

void AndroidLogger::debug(const std::string& tag, const std::string& message) {
    __android_log_print(ANDROID_LOG_DEBUG, tag.c_str(), "%s", message.c_str());
}

void AndroidLogger::warn(const std::string& tag, const std::string& message) {
    __android_log_print(ANDROID_LOG_WARN, tag.c_str(), "%s", message.c_str());
}

void AndroidLogger::error(const std::string& tag, const std::string& message) {
    __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), "%s", message.c_str());
}
#else
// Dummy implementation for desktop builds so it compiles without NDK log headers
#include "Core/AndroidLogger.h"

void AndroidLogger::info(const std::string&, const std::string&) {}
void AndroidLogger::debug(const std::string&, const std::string&) {}
void AndroidLogger::warn(const std::string&, const std::string&) {}
void AndroidLogger::error(const std::string&, const std::string&) {}
#endif
