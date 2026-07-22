#include <gtest/gtest.h>
#include <map>
#include "Network/IHaController.h"
#include "SensorBridge.h"
#include "HomeDeviceBase.h"
#include "DeviceModel.h"
#include "IDeviceFactory.h"
#include "ISettingsManager.h"
#include "SwitchableComponent.h"
#include "Commands/GenericHaCommand.h"

class MockSettingsManager : public ISettingsManager {
public:
    std::map<std::string, std::string> aliases;
    std::map<std::string, bool> visibility;

    void saveAlias(const std::string& entityId, const std::string& alias) override {
        aliases[entityId] = alias;
    }
    std::string getAlias(const std::string& entityId, const std::string& defaultAlias) override {
        auto it = aliases.find(entityId);
        if (it != aliases.end()) {
            return it->second;
        }
        return defaultAlias;
    }
    void saveVisibility(const std::string& entityId, bool visible) override {
        visibility[entityId] = visible;
    }
    bool getVisibility(const std::string& entityId, bool defaultVisible) override {
        auto it = visibility.find(entityId);
        if (it != visibility.end()) {
            return it->second;
        }
        return defaultVisible;
    }
};

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

class DummyDevice : public HomeDeviceBase {
public:
    DummyDevice(const std::string& id, const std::string& topic)
        : HomeDeviceBase(id, topic, DeviceType::Light) {}
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

TEST_F(SensorBridgeTest, PublishCommand_HandlesHierarchicalTopics) {
    auto dummy = std::make_unique<DummyDevice>("LightLiving", "light.living_room");
    dummy->addComponent(std::make_unique<SwitchableComponent>(dummy.get()));
    deviceModel.addDevice(std::move(dummy));
    bridge.publishCommand("light.living_room", "ON");
    
    EXPECT_EQ(mockHa.lastDomain, "light");
    EXPECT_EQ(mockHa.lastService, "turn_on");
    EXPECT_EQ(mockHa.lastEntityId, "light.living_room");
}

TEST_F(SensorBridgeTest, LanguageStorageAndRetrieval) {
    EXPECT_EQ(bridge.getSavedLanguage(), "system");
    
    bridge.saveLanguage("es");
    EXPECT_EQ(bridge.getSavedLanguage(), "es");
    
    bridge.saveLanguage("de");
    EXPECT_EQ(bridge.getSavedLanguage(), "de");
}

TEST_F(SensorBridgeTest, HaCredentialsAndUrls) {
    EXPECT_EQ(bridge.getSavedHaUrl(), "");
    EXPECT_EQ(bridge.getSavedHaToken(), "");
    
    bridge.saveHaCredentials("ws://192.168.1.100:8123/api/websocket", "my_secret_token");
    EXPECT_EQ(bridge.getSavedHaUrl(), "ws://192.168.1.100:8123/api/websocket");
    EXPECT_EQ(bridge.getSavedHaToken(), "my_secret_token");
    
    EXPECT_EQ(bridge.getHaMapUrl("camera.living_room"), "http://192.168.1.100:8123/api/camera_proxy/camera.living_room?token=my_secret_token");
}
