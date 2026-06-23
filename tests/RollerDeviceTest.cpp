#include <gtest/gtest.h>
#include "RollerDevice.h"

// Fixture para evitar repetir código en cada test
class RollerDeviceTest : public ::testing::Test {
protected:
    std::string deviceId = "test_roller";
    std::string deviceTopic = "home/roller/test_roller";
    RollerDevice roller{deviceId, deviceTopic};
};

TEST_F(RollerDeviceTest, Initialization_KeepsCorrectId) {
    EXPECT_EQ(roller.getId(), "test_roller");
    EXPECT_FLOAT_EQ(roller.getValue(), 0.0f);
}

TEST_F(RollerDeviceTest, MessageReceived_CleanNumber_UpdatesValue) {
    QJsonObject attrs;
    attrs["current_position"] = 50;
    roller.updateState("open", attrs);
    EXPECT_FLOAT_EQ(roller.getValue(), 0.5f); // 50 / 100 = 0.5
}

TEST_F(RollerDeviceTest, MessageReceived_CleanNumber_IsMoving) {
    QJsonObject attrs;
    attrs["current_position"] = 70;
    roller.updateState("opening", attrs);
    EXPECT_TRUE(roller.isMoving());
    attrs["current_position"] = 0;
    roller.updateState("closing", attrs);
    EXPECT_TRUE(roller.isMoving());
    attrs["current_position"] = 20;
    roller.updateState("open", attrs);
    EXPECT_FALSE(roller.isMoving());
}
