#include "Core/Log.h"

std::shared_ptr<ILogger> Log::m_logger = nullptr;

#if !defined(QT_NO_DEBUG) || defined(_DEBUG) || defined(DEBUG) || defined(__ANDROID__)
unsigned int Log::m_logMask = LogLevelAll;
#else
unsigned int Log::m_logMask = LogLevelWarn | LogLevelError;
#endif

void Log::setLogger(std::shared_ptr<ILogger> logger) {
    m_logger = logger;
}

void Log::setLogMask(unsigned int mask) {
    m_logMask = mask;
}

unsigned int Log::getLogMask() {
    return m_logMask;
}

void Log::info(const std::string& tag, const std::string& message) {
    if (m_logger && (m_logMask & LogLevelInfo)) {
        m_logger->info(tag, message);
    }
}

void Log::debug(const std::string& tag, const std::string& message) {
    if (m_logger && (m_logMask & LogLevelDebug)) {
        m_logger->debug(tag, message);
    }
}

void Log::warn(const std::string& tag, const std::string& message) {
    if (m_logger && (m_logMask & LogLevelWarn)) {
        m_logger->warn(tag, message);
    }
}

void Log::error(const std::string& tag, const std::string& message) {
    if (m_logger && (m_logMask & LogLevelError)) {
        m_logger->error(tag, message);
    }
}
