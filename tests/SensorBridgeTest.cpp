#include <gtest/gtest.h>
#include "Network/IHaController.h"
#include "SensorBridge.h"
#include "HomeDeviceBase.h"
#include "DeviceModel.h"
#include "IDeviceFactory.h"
#include "ISettingsManager.h"

class MockSettingsManager : public ISettingsManager {
public:
    void saveAlias(const std::string& entityId, const std::string& alias) override {}
    std::string getAlias(const std::string& entityId, const std::string& defaultAlias) override {
        return defaultAlias;
    }
    void saveVisibility(const std::string& entityId, bool visible) override {}
    bool getVisibility(const std::string& entityId, bool defaultVisible) override {
        return defaultVisible;
    }
};

// Mock para interceptar las llamadas al controlador MQTT desde el Bridge
class MockHaController : public IHaController {
public:
    std::string lastDomain;
    std::string lastService;
    std::string lastEntityId;
    QJsonObject lastServiceData;

    void connectToHa(const std::string& url, const std::string& token) override {}
    void callService(const std::string& domain, 
                             const std::string& service, 
                             const std::string& entityId, 
                             const QJsonObject& serviceData = {}) override
    {
        lastDomain = domain;
        lastService = service;
        lastEntityId = entityId;
        lastServiceData = serviceData;
    }
};

// Clase Dummy de dispositivo para el test
class DummyDevice : public HomeDeviceBase {
public:
    using HomeDeviceBase::HomeDeviceBase;
    void updateState(const std::string&, const QJsonObject&) override {}
    DeviceType getType() const override { return DeviceType::Light; }
    void prepareForCommand(const std::string&) override {}
};

class MockDeviceFactory : public IDeviceFactory {
public:
    std::unique_ptr<HomeDeviceBase> create(const std::string& type, const std::string& id, const std::string& topic) override {
        return nullptr;
    }
    void registerType(const std::string& type, std::function<std::unique_ptr<HomeDeviceBase>(const std::string&, const std::string&)> creator) override {}
};

class SensorBridgeTest : public ::testing::Test {
protected:
    MockDeviceFactory mockFactory;
    DeviceModel deviceModel;
    MockHaController mockHa;
    MockSettingsManager mockSettings;
    SensorBridge bridge{mockFactory, deviceModel, mockHa, mockSettings};
};

// Verificamos que funciona con diferentes niveles de tópicos
TEST_F(SensorBridgeTest, PublishCommand_HandlesHierarchicalTopics) {
    auto dummy = std::make_unique<DummyDevice>("LightLiving", "light.living_room");
    deviceModel.addDevice(std::move(dummy));
    bridge.publishCommand("light.living_room", "ON");
    
    EXPECT_EQ(mockHa.lastDomain, "light");
    EXPECT_EQ(mockHa.lastService, "turn_on");
    EXPECT_EQ(mockHa.lastEntityId, "light.living_room");
}
