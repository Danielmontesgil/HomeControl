#include <gtest/gtest.h>
#include "MqttController.h"
#include "IMqttListener.h"

// Listener Falso (Mock) para espiar lo que hace el MqttController
class MockMqttListener : public IMqttListener {
public:
    std::string lastTopic;
    std::string lastPayload;
    int callCount = 0;

    void onMessageReceived(const std::string& topic, const std::string& payload) override {
        lastTopic = topic;
        lastPayload = payload;
        callCount++;
    }
};

class MqttControllerTest : public ::testing::Test {
protected:
    // Creamos el controlador, pero no llamamos a connect() para no levantar la red
    MqttController controller{"tcp://localhost:1883", "test_client"};
};

// 1. Verificamos que los oyentes globales (*) reciben todo
TEST_F(MqttControllerTest, GlobalListener_ReceivesAllMessages) {
    MockMqttListener globalLogger;
    controller.addListener("*", &globalLogger);

    auto msg = mqtt::make_message("home/random/topic", "Hello");
    controller.message_arrived(msg);

    EXPECT_EQ(globalLogger.callCount, 1);
    EXPECT_EQ(globalLogger.lastTopic, "home/random/topic");
    EXPECT_EQ(globalLogger.lastPayload, "Hello");
}

// 2. Verificamos el Prefix Matching (Enrutamiento Jerárquico)
TEST_F(MqttControllerTest, PrefixMatching_RoutesToCorrectListeners) {
    MockMqttListener lamp1;
    MockMqttListener lamp2;
    MockMqttListener kitchenLamp;

    // Los dispositivos se registran con sus IDs especificos
    controller.addListener("home/light/living/1", &lamp1);
    controller.addListener("home/light/living/2", &lamp2);
    controller.addListener("home/light/kitchen/1", &kitchenLamp);

    // Enviamos un mensaje de GRUPO al salon
    auto msg = mqtt::make_message("home/light/living", "ON");
    controller.message_arrived(msg);

    // Ambas luces del salon deben enterarse, la cocina NO
    EXPECT_EQ(lamp1.callCount, 1);
    EXPECT_EQ(lamp1.lastPayload, "ON");
    EXPECT_EQ(lamp2.callCount, 1);
    EXPECT_EQ(kitchenLamp.callCount, 0);
}

// 3. Verificamos que los mensajes especificos NO afectan a otros
TEST_F(MqttControllerTest, SpecificMessage_DoesNotLeakToSiblings) {
    MockMqttListener lamp1;
    MockMqttListener lamp2;
    
    controller.addListener("home/light/living/1", &lamp1);
    controller.addListener("home/light/living/2", &lamp2);

    // Enviamos a una luz especifica
    auto msg = mqtt::make_message("home/light/living/1", "OFF");
    controller.message_arrived(msg);

    // Solo la lamp1 debe reaccionar
    EXPECT_EQ(lamp1.callCount, 1);
    EXPECT_EQ(lamp1.lastPayload, "OFF");
    EXPECT_EQ(lamp2.callCount, 0);
}
