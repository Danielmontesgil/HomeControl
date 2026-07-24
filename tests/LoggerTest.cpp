#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Core/Log.h"
#include "Core/ILogger.h"
#include "Core/ConsoleLogger.h"

using ::testing::_;
using ::testing::Exactly;

class MockLogger : public ILogger {
public:
    MOCK_METHOD(void, info, (const std::string& tag, const std::string& message), (override));
    MOCK_METHOD(void, debug, (const std::string& tag, const std::string& message), (override));
    MOCK_METHOD(void, warn, (const std::string& tag, const std::string& message), (override));
    MOCK_METHOD(void, error, (const std::string& tag, const std::string& message), (override));
};

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockLogger = std::make_shared<MockLogger>();
        Log::setLogger(mockLogger);
        Log::setLogMask(LogLevelAll);
    }
    
    void TearDown() override {
        Log::setLogger(nullptr);
    }
    
    std::shared_ptr<MockLogger> mockLogger;
};

TEST_F(LoggerTest, MaskChangeWorks) {
    Log::setLogMask(LogLevelWarn | LogLevelError);
    EXPECT_EQ(Log::getLogMask(), LogLevelWarn | LogLevelError);
}

TEST_F(LoggerTest, BitmaskFiltering) {
    Log::setLogMask(LogLevelWarn | LogLevelError);
    
    EXPECT_CALL(*mockLogger, info(_, _)).Times(0);
    EXPECT_CALL(*mockLogger, debug(_, _)).Times(0);
    EXPECT_CALL(*mockLogger, warn("TestTag", "WarnMsg")).Times(Exactly(1));
    EXPECT_CALL(*mockLogger, error("TestTag", "ErrorMsg")).Times(Exactly(1));
    
    Log::info("TestTag", "InfoMsg");
    Log::debug("TestTag", "DebugMsg");
    Log::warn("TestTag", "WarnMsg");
    Log::error("TestTag", "ErrorMsg");
}

TEST_F(LoggerTest, PassesCorrectTagAndMessage) {
    Log::setLogMask(LogLevelAll);
    
    EXPECT_CALL(*mockLogger, info("TagInfo", "MsgInfo")).Times(1);
    EXPECT_CALL(*mockLogger, debug("TagDebug", "MsgDebug")).Times(1);
    EXPECT_CALL(*mockLogger, warn("TagWarn", "MsgWarn")).Times(1);
    EXPECT_CALL(*mockLogger, error("TagError", "MsgError")).Times(1);
    
    Log::info("TagInfo", "MsgInfo");
    Log::debug("TagDebug", "MsgDebug");
    Log::warn("TagWarn", "MsgWarn");
    Log::error("TagError", "MsgError");
}

TEST_F(LoggerTest, ConsoleLoggerSanity) {
    auto consoleLogger = std::make_shared<ConsoleLogger>();
    Log::setLogger(consoleLogger);
    Log::setLogMask(LogLevelAll);
    
    testing::internal::CaptureStdout();
    Log::info("TestTag", "TestMsg");
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_NE(output.find("[INFO]"), std::string::npos);
    EXPECT_NE(output.find("TestTag"), std::string::npos);
    EXPECT_NE(output.find("TestMsg"), std::string::npos);
    
    testing::internal::CaptureStderr();
    Log::error("ErrTag", "ErrMsg");
    std::string errOutput = testing::internal::GetCapturedStderr();
    
    EXPECT_NE(errOutput.find("[ERROR]"), std::string::npos);
    EXPECT_NE(errOutput.find("ErrTag"), std::string::npos);
    EXPECT_NE(errOutput.find("ErrMsg"), std::string::npos);
}
