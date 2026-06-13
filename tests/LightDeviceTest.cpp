#include <gtest/gtest.h>
#include "LightDevice.h"

class LightDeviceTest : public ::testing::Test {
protected:
    std::string deviceId = "test_light";
    LightDevice light{deviceId};
};

// 1. Estado inicial
TEST_F(LightDeviceTest, Initialization_IsOff) {
    EXPECT_FLOAT_EQ(light.getValue(), 0.0f);
}

// 2. Encendido
TEST_F(LightDeviceTest, MessageReceived_ON_TurnsLightOn) {
    light.onMessageReceived("test/topic", "ON");
    EXPECT_FLOAT_EQ(light.getValue(), 1.0f);
}

// 3. Apagado
TEST_F(LightDeviceTest, MessageReceived_OFF_TurnsLightOff) {
    light.onMessageReceived("test/topic", "ON"); // Encendemos primero
    light.onMessageReceived("test/topic", "OFF");
    EXPECT_FLOAT_EQ(light.getValue(), 0.0f);
}

// 4. Case Sensitivity (Si el protocolo exige mayúsculas)
TEST_F(LightDeviceTest, MessageReceived_LowerCaseOn_IsIgnored) {
    light.onMessageReceived("test/topic", "on");
    EXPECT_FLOAT_EQ(light.getValue(), 0.0f); // Debería seguir apagada
}
