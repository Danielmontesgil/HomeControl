#include <gtest/gtest.h>
#include "SensorBridge.h"
#include "IMqttController.h"
#include "HomeDeviceBase.h"

// Mock para interceptar las llamadas al controlador MQTT desde el Bridge
class MockMqttController : public IMqttController {
public:
    std::string lastTopic;
    std::string lastPayload;
    int publishCount = 0;

    void connect() override {}
    void publish(const std::string& topic, const std::string& payload) override {
        lastTopic = topic;
        lastPayload = payload;
        publishCount++;
    }
    void subscribe(const std::string& topic) override {}
    void addListener(const std::string& topic, IMqttListener* listener) override {}
};

// Clase Dummy de dispositivo para el test
class DummyDevice : public HomeDeviceBase {
public:
    using HomeDeviceBase::HomeDeviceBase;
    void onMessageReceived(const std::string&, const std::string&) override {}
};

class SensorBridgeTest : public ::testing::Test {
protected:
    MockMqttController mockMqtt;
    DummyDevice light{"light_id"};
    DummyDevice roller{"roller_id"};
    // No necesitamos pasar parent (nullptr por defecto)
    SensorBridge bridge{mockMqtt, light, roller};
};

// Verificamos que publishCommand envía los datos correctos al controlador
TEST_F(SensorBridgeTest, PublishCommand_DelegatesToMqttController) {
    QString topic = "home/light/all";
    QString payload = "OFF";
    
    bridge.publishCommand(topic, payload);
    
    EXPECT_EQ(mockMqtt.publishCount, 1);
    EXPECT_EQ(mockMqtt.lastTopic, "home/light/all");
    EXPECT_EQ(mockMqtt.lastPayload, "OFF");
}

// Verificamos que funciona con diferentes niveles de tópicos
TEST_F(SensorBridgeTest, PublishCommand_HandlesHierarchicalTopics) {
    bridge.publishCommand("home/roller/living", "50");
    
    EXPECT_EQ(mockMqtt.lastTopic, "home/roller/living");
    EXPECT_EQ(mockMqtt.lastPayload, "50");
}
