#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>
#include "HaWebSocketController.h"

class HaWebSocketControllerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char appName[] = "HaWebSocketControllerTest";
            static char* argv[] = { appName, nullptr };
            new QCoreApplication(argc, argv);
        }
    }

    HaWebSocketController controller;
};

// 1. Verifies that retryAttemptCount() increments progressively with each disconnection simulation
TEST_F(HaWebSocketControllerTest, RetryAttemptCountIncrementsProgressivelyOnDisconnections) {
    EXPECT_EQ(controller.retryAttemptCount(), 0);

    for (int i = 1; i <= 5; ++i) {
        bool invoked = QMetaObject::invokeMethod(&controller, "onDisconnected");
        ASSERT_TRUE(invoked);
        EXPECT_EQ(controller.retryAttemptCount(), i);
    }

    // Continue up to maximum capped attempt count (10)
    for (int i = 6; i <= 15; ++i) {
        QMetaObject::invokeMethod(&controller, "onDisconnected");
        int expectedCount = (i > 10) ? 10 : i;
        EXPECT_EQ(controller.retryAttemptCount(), expectedCount);
    }
}

// 2. Verifies that receiving auth_ok resets retryAttemptCount to 0 and sets isAuthenticated to true
TEST_F(HaWebSocketControllerTest, ResetsRetryCountOnAuthOk) {
    // Simulate several disconnections to accumulate retry attempts
    for (int i = 0; i < 3; ++i) {
        QMetaObject::invokeMethod(&controller, "onDisconnected");
    }
    EXPECT_EQ(controller.retryAttemptCount(), 3);
    EXPECT_FALSE(controller.isAuthenticated());

    bool connectedSignalEmitted = false;
    QObject::connect(&controller, &HaWebSocketController::connected, [&connectedSignalEmitted]() {
        connectedSignalEmitted = true;
    });

    // Simulate receiving auth_ok from HA
    QString authOkMessage = QStringLiteral(R"({"type": "auth_ok", "ha_version": "2024.1.0"})");
    bool invoked = QMetaObject::invokeMethod(&controller, "onTextMessageReceived", Q_ARG(QString, authOkMessage));
    ASSERT_TRUE(invoked);

    EXPECT_EQ(controller.retryAttemptCount(), 0);
    EXPECT_TRUE(controller.isAuthenticated());
    EXPECT_TRUE(connectedSignalEmitted);
}

// 3. Verifies that connectToHa resets retryAttemptCount to 0
TEST_F(HaWebSocketControllerTest, ResetsRetryCountOnConnectToHa) {
    for (int i = 0; i < 4; ++i) {
        QMetaObject::invokeMethod(&controller, "onDisconnected");
    }
    EXPECT_EQ(controller.retryAttemptCount(), 4);

    controller.connectToHa("ws://127.0.0.1:8123/api/websocket", "dummy_token");

    EXPECT_EQ(controller.retryAttemptCount(), 0);
    EXPECT_FALSE(controller.isAuthenticated());
}

// 4. Verifies that receiving auth_invalid stops reconnect timer and prevents subsequent retries
TEST_F(HaWebSocketControllerTest, StopsTimerAndPreventsRetryOnAuthInvalid) {
    // Trigger disconnection so timer becomes active
    QMetaObject::invokeMethod(&controller, "onDisconnected");
    EXPECT_EQ(controller.retryAttemptCount(), 1);

    QTimer* timer = controller.findChild<QTimer*>();
    ASSERT_NE(timer, nullptr);
    EXPECT_TRUE(timer->isActive());

    // Simulate receiving auth_invalid message
    QString authInvalidMessage = QStringLiteral(R"({"type": "auth_invalid", "message": "Invalid access token"})");
    bool invoked = QMetaObject::invokeMethod(&controller, "onTextMessageReceived", Q_ARG(QString, authInvalidMessage));
    ASSERT_TRUE(invoked);

    // Verify timer is stopped and retry count is reset
    EXPECT_FALSE(timer->isActive());
    EXPECT_EQ(controller.retryAttemptCount(), 0);
    EXPECT_FALSE(controller.isAuthenticated());

    // Simulate another disconnection event; timer should NOT restart because shouldReconnect is false
    QMetaObject::invokeMethod(&controller, "onDisconnected");
    EXPECT_FALSE(timer->isActive());
}
