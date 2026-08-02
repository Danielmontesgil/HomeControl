#include <gtest/gtest.h>
#include <QJsonObject>
#include <QString>
#include "Devices/DishwasherComponent.h"
#include "HomeDeviceBase.h"
#include "Network/IHaController.h"
#include "Commands/GenericHaCommand.h"
#include "SensorBridge.h"
#include "DeviceModel.h"
#include "DeviceFactory.h"
#include "ISettingsManager.h"
#include "Devices/ILicenseManager.h"
#include <map>
#include <memory>
#include <vector>

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

class MockLicenseManager : public ILicenseManager {
public:
    bool isFeatureLicensed(const std::string& featureId) const override { return true; }
    bool activateFeature(const std::string& featureId, const std::string& licenseKey) override { return true; }
};

class DishwasherComponentTest : public ::testing::Test {
protected:
    HomeDeviceBase device;
    DishwasherComponent component;
    MockHaController controller;

    DishwasherComponentTest() : device("dishwasher.test", "dishwasher.topic", DeviceType::Dishwasher), component(&device) {}
};

TEST_F(DishwasherComponentTest, InitialState) {
    EXPECT_FALSE(component.isDoorOpen());
    EXPECT_EQ(component.getOperationState(), "Off");
    EXPECT_EQ(component.getRemainingTime(), 0);
    EXPECT_EQ(component.getSelectedProgram(), "None");
}

TEST_F(DishwasherComponentTest, UpdateState_Door) {
    QJsonObject attrs;
    attrs["entity_id"] = "binary_sensor.bosch_lavavajillas_door";
    
    component.updateState(attrs, "on");
    EXPECT_TRUE(component.isDoorOpen());
    
    component.updateState(attrs, "off");
    EXPECT_FALSE(component.isDoorOpen());
    
    component.updateState(attrs, "open");
    EXPECT_TRUE(component.isDoorOpen());

    component.updateState(attrs, "abierto");
    EXPECT_TRUE(component.isDoorOpen());
    
    component.updateState(attrs, "true");
    EXPECT_TRUE(component.isDoorOpen());
}

TEST_F(DishwasherComponentTest, UpdateState_OperationState) {
    QJsonObject attrs;
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    
    component.updateState(attrs, "Run");
    EXPECT_EQ(component.getOperationState(), "Run");

    attrs["entity_id"] = "sensor.bosch_lavavajillas_estado_de_operacion";
    component.updateState(attrs, "Finished");
    EXPECT_EQ(component.getOperationState(), "Finished");
}

TEST_F(DishwasherComponentTest, UpdateState_RemainingTime) {
    QJsonObject attrs;
    attrs["entity_id"] = "sensor.bosch_lavavajillas_remaining_program_time";
    
    component.updateState(attrs, "7200");
    EXPECT_EQ(component.getRemainingTime(), 120);

    attrs["entity_id"] = "sensor.bosch_lavavajillas_tiempo_restante";
    component.updateState(attrs, "3600");
    EXPECT_EQ(component.getRemainingTime(), 60);
}

TEST_F(DishwasherComponentTest, UpdateState_SelectedProgram) {
    QJsonObject attrs;
    attrs["entity_id"] = "sensor.bosch_lavavajillas_selected_program";
    
    component.updateState(attrs, "Eco 50°");
    EXPECT_EQ(component.getSelectedProgram(), "Eco 50°");

    attrs["entity_id"] = "sensor.bosch_lavavajillas_programa_seleccionado";
    component.updateState(attrs, "Auto 45-65°");
    EXPECT_EQ(component.getSelectedProgram(), "Auto 45-65°");
}

TEST_F(DishwasherComponentTest, ParseCommand) {
    auto cmdOn = component.parseCommand("ON", controller);
    ASSERT_NE(cmdOn, nullptr);
    cmdOn->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_on");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_power_status");

    auto cmdOff = component.parseCommand("OFF", controller);
    ASSERT_NE(cmdOff, nullptr);
    cmdOff->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_off");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_power_status");

    auto cmdStart = component.parseCommand("START", controller);
    ASSERT_NE(cmdStart, nullptr);
    cmdStart->execute();
    EXPECT_EQ(controller.lastDomain, "button");
    EXPECT_EQ(controller.lastService, "press");
    EXPECT_EQ(controller.lastEntityId, "button.bosch_lavavajillas_start");
}

TEST_F(DishwasherComponentTest, SensorBridgeIntegration) {
    DeviceModel model;
    MockSettingsManager settings;
    DeviceFactory factory;

    factory.registerType("Dishwasher", [](const std::string& id, const std::string& topic) {
        auto dev = std::make_unique<HomeDeviceBase>(id, topic, DeviceType::Dishwasher);
        dev->addComponent(std::make_unique<DishwasherComponent>(dev.get()));
        return dev;
    });

    MockHaController haController;
    MockLicenseManager mockLicense;
    SensorBridge bridge(factory, model, haController, settings, mockLicense);

    QJsonObject attrs;
    
    // Simulate discovering the door
    attrs["entity_id"] = "binary_sensor.bosch_lavavajillas_door";
    attrs["friendly_name"] = "Door";
    bridge.onDeviceDiscovered("binary_sensor", "binary_sensor.bosch_lavavajillas_door", "Door", "off", attrs);

    // Simulate discovering the remaining time
    attrs["entity_id"] = "sensor.bosch_lavavajillas_remaining_program_time";
    attrs["friendly_name"] = "Remaining Time";
    bridge.onDeviceDiscovered("sensor", "sensor.bosch_lavavajillas_remaining_program_time", "Remaining Time", "7200", attrs);

    // We should have a unified dishwasher device named dishwasher.bosch_lavavajillas
    auto dwDevice = model.findByTopic("dishwasher.bosch_lavavajillas");
    
    ASSERT_NE(dwDevice, nullptr);

    EXPECT_TRUE(dwDevice->hasComponent("dishwasher"));
    auto dwComponent = dwDevice->getComponent("dishwasher");
    EXPECT_EQ(dwComponent->getProperty("dishwasherRemainingTime").toInt(), 120);
    EXPECT_EQ(dwComponent->getProperty("dishwasherDoorOpen").toBool(), false);
}
