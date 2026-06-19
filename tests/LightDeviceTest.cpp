#include <gtest/gtest.h>
#include "LightDevice.h"

class LightDeviceTest : public ::testing::Test {
protected:
    std::string deviceId = "test_light";
    std::string deviceTopic = "home/light/test_light";
    LightDevice light{deviceId, deviceTopic};
};

// 1. Estado inicial
TEST_F(LightDeviceTest, Initialization_IsOff) {
    EXPECT_FALSE(light.isOn());
}

// 2. Encendido
TEST_F(LightDeviceTest, MessageReceived_ON_TurnsLightOn) {
    light.onMessageReceived("test/topic", "ON");
    EXPECT_TRUE(light.isOn());
}

// 3. Apagado
TEST_F(LightDeviceTest, MessageReceived_OFF_TurnsLightOff) {
    light.onMessageReceived("test/topic", "ON"); // Encendemos primero
    light.onMessageReceived("test/topic", "OFF");
    EXPECT_FALSE(light.isOn());
}

// 4. Case Sensitivity (Si el protocolo exige mayúsculas)
TEST_F(LightDeviceTest, MessageReceived_LowerCaseOn_IsIgnored) {
    light.onMessageReceived("test/topic", "on");
    EXPECT_FALSE(light.isOn()); // Debería seguir apagada
}
