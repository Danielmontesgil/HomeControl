#include <gtest/gtest.h>
#include "RollerDevice.h"

// Fixture para evitar repetir código en cada test
class RollerDeviceTest : public ::testing::Test {
protected:
    std::string deviceId = "test_roller";
    RollerDevice roller{deviceId};
};

// 1. Verificación del ID
TEST_F(RollerDeviceTest, Initialization_KeepsCorrectId) {
    EXPECT_EQ(roller.getId(), "test_roller");
    EXPECT_FLOAT_EQ(roller.getValue(), 0.0f);
}

// 2. Parseo de números limpios
TEST_F(RollerDeviceTest, MessageReceived_CleanNumber_UpdatesValue) {
    roller.onMessageReceived("test/topic", "50");
    EXPECT_FLOAT_EQ(roller.getValue(), 0.5f); // 50 / 100 = 0.5
}

// 3. Saneamiento: Eliminar símbolo '%'
TEST_F(RollerDeviceTest, MessageReceived_NumberWithPercent_UpdatesValue) {
    roller.onMessageReceived("test/topic", "75.5%");
    EXPECT_FLOAT_EQ(roller.getValue(), 0.755f);
}

// 4. Saneamiento: Eliminar espacios y texto adicional
TEST_F(RollerDeviceTest, MessageReceived_DirtyString_UpdatesValue) {
    roller.onMessageReceived("test/topic", " Pos: 33.3 % ");
    EXPECT_FLOAT_EQ(roller.getValue(), 0.333f);
}

// 5. Manejo de errores: String vacío
TEST_F(RollerDeviceTest, MessageReceived_EmptyString_IgnoresValue) {
    roller.onMessageReceived("test/topic", "50"); // Seteamos a 0.5 primero
    roller.onMessageReceived("test/topic", "");
    EXPECT_FLOAT_EQ(roller.getValue(), 0.5f); // No debería cambiar
}

// 6. Manejo de errores: String no numérico (El try-catch debe proteger)
TEST_F(RollerDeviceTest, MessageReceived_InvalidString_DoesNotCrash) {
    roller.onMessageReceived("test/topic", "50"); // Seteamos a 0.5 primero
    EXPECT_NO_THROW({
        roller.onMessageReceived("test/topic", "OPEN");
    });
    EXPECT_FLOAT_EQ(roller.getValue(), 0.5f); // No debería cambiar
}
