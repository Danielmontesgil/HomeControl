#include <gtest/gtest.h>
#include <QJsonObject>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include "Devices/DishwasherComponent.h"
#include "Devices/DeviceMappingManager.h"
#include "HomeDeviceBase.h"
#include "Network/IHaController.h"
#include "Commands/GenericHaCommand.h"
#include "SensorBridge.h"
#include "DeviceModel.h"
#include "DeviceFactory.h"
#include "ISettingsManager.h"
#include "Devices/ILicenseManager.h"
#include "Devices/SwitchableComponent.h"
#include <map>
#include <memory>
#include <vector>
#include <iostream>

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
    QVariantMap mappings;
    mappings["door"] = "binary_sensor.bosch_lavavajillas_door";
    component.setEntityMappings(mappings);

    QJsonObject attrs;
    attrs["entity_id"] = "binary_sensor.bosch_lavavajillas_door";
    
    component.updateState(attrs, "on");
    EXPECT_TRUE(component.isDoorOpen());
    
    component.updateState(attrs, "off");
    EXPECT_FALSE(component.isDoorOpen());
    
    component.updateState(attrs, "open");
    EXPECT_TRUE(component.isDoorOpen());
    
    component.updateState(attrs, "true");
    EXPECT_TRUE(component.isDoorOpen());
}

TEST_F(DishwasherComponentTest, UpdateState_OperationState) {
    QVariantMap mappings;
    mappings["operation_state"] = "sensor.bosch_lavavajillas_operation_state";
    component.setEntityMappings(mappings);

    QJsonObject attrs;
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    
    component.updateState(attrs, "Run");
    EXPECT_EQ(component.getOperationState(), "Run");
}

TEST_F(DishwasherComponentTest, UpdateState_RemainingTime) {
    QVariantMap mappings;
    mappings["remaining_time"] = "sensor.bosch_lavavajillas_remaining_program_time";
    mappings["operation_state"] = "sensor.bosch_lavavajillas_operation_state";
    component.setEntityMappings(mappings);

    QJsonObject attrs;
    
    // 1. If operation state is not running (e.g. Ready), getRemainingTime() must return 0
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    component.updateState(attrs, "Ready");
    
    attrs["entity_id"] = "sensor.bosch_lavavajillas_remaining_program_time";
    component.updateState(attrs, "7200");
    EXPECT_EQ(component.getRemainingTime(), 0);

    // 2. Set operation state to "Run" (active wash) to verify remaining time parsing
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    component.updateState(attrs, "Run");
    
    attrs["entity_id"] = "sensor.bosch_lavavajillas_remaining_program_time";
    component.updateState(attrs, "7200");
    EXPECT_EQ(component.getRemainingTime(), 120);

    // Test ISO timestamp parsing (e.g., 60 minutes in the future)
    QString isoStr = QDateTime::currentDateTimeUtc().addSecs(3600).toString(Qt::ISODate);
    component.updateState(attrs, isoStr);
    EXPECT_NEAR(component.getRemainingTime(), 60, 1);

    // Test duration format H:MM:SS (e.g., 4:40:00 -> 280 minutes)
    component.updateState(attrs, "4:40:00");
    EXPECT_EQ(component.getRemainingTime(), 280);

    // Test duration format MM:SS or H:MM (e.g., 04:40 -> 280 minutes)
    component.updateState(attrs, "04:40");
    EXPECT_EQ(component.getRemainingTime(), 280);
}

TEST_F(DishwasherComponentTest, RemainingTime_OnlyWhenRunning) {
    QVariantMap mappings;
    mappings["remaining_time"] = "sensor.bosch_lavavajillas_remaining_program_time";
    mappings["operation_state"] = "sensor.bosch_lavavajillas_operation_state";
    component.setEntityMappings(mappings);

    QJsonObject attrs;
    
    // Set time remaining first
    attrs["entity_id"] = "sensor.bosch_lavavajillas_remaining_program_time";
    component.updateState(attrs, "3600"); // 60 minutes

    // Should return 0 if operation state is empty/unset
    EXPECT_EQ(component.getRemainingTime(), 0);

    // Should return 60 if state is "Run"
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    component.updateState(attrs, "Run");
    EXPECT_EQ(component.getRemainingTime(), 60);

    // Should return 60 if state is "running" (case insensitive)
    component.updateState(attrs, "running");
    EXPECT_EQ(component.getRemainingTime(), 60);

    // Should return 0 if state is "Finished"
    component.updateState(attrs, "Finished");
    EXPECT_EQ(component.getRemainingTime(), 0);
}

TEST_F(DishwasherComponentTest, ParseCommand_SelectProgramUsingProgramsEntity) {
    QVariantMap mappings;
    mappings["selected_program"] = "sensor.bosch_lavavajillas_selected_program";
    mappings["programs"] = "select.bosch_lavavajillas_programs";
    component.setEntityMappings(mappings);

    // When parsing SELECT_PROGRAM, it must target the interactive programs select entity
    auto cmd = component.parseCommand("SELECT_PROGRAM:Dishcare.Dishwasher.Program.Eco50", controller);
    ASSERT_NE(cmd, nullptr);
    cmd->execute();
    
    EXPECT_EQ(controller.lastDomain, "select");
    EXPECT_EQ(controller.lastService, "select_option");
    EXPECT_EQ(controller.lastEntityId, "select.bosch_lavavajillas_programs");
    
    QJsonObject expectedData;
    expectedData["option"] = "Dishcare.Dishwasher.Program.Eco50";
    EXPECT_EQ(controller.lastServiceData, expectedData);
}

TEST_F(DishwasherComponentTest, ResetToDefaults_OnTurnOff) {
    QVariantMap mappings;
    mappings["power"] = "switch.bosch_lavavajillas_power_status";
    mappings["operation_state"] = "sensor.bosch_lavavajillas_operation_state";
    mappings["extradry"] = "switch.bosch_lavavajillas_extradry";
    mappings["half_load"] = "switch.bosch_lavavajillas_half_load";
    mappings["speedperfect"] = "switch.bosch_lavavajillas_speedperfect";
    mappings["selected_program"] = "sensor.bosch_lavavajillas_selected_program";
    mappings["remaining_time"] = "sensor.bosch_lavavajillas_remaining_program_time";
    component.setEntityMappings(mappings);

    QJsonObject attrs;
    
    // 1. Set some active/true values
    attrs["entity_id"] = "switch.bosch_lavavajillas_extradry";
    component.updateState(attrs, "on");
    
    attrs["entity_id"] = "switch.bosch_lavavajillas_half_load";
    component.updateState(attrs, "on");
    
    attrs["entity_id"] = "switch.bosch_lavavajillas_speedperfect";
    component.updateState(attrs, "on");
    
    attrs["entity_id"] = "sensor.bosch_lavavajillas_selected_program";
    component.updateState(attrs, "Quick Wash");
    
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    component.updateState(attrs, "Run");
    
    attrs["entity_id"] = "sensor.bosch_lavavajillas_remaining_program_time";
    component.updateState(attrs, "3600"); // 60 min
    
    // Verify they are saved
    EXPECT_TRUE(component.isExtraDry());
    EXPECT_TRUE(component.isHalfLoad());
    EXPECT_TRUE(component.isSpeedPerfect());
    EXPECT_EQ(component.getSelectedProgram(), "Quick Wash");
    EXPECT_EQ(component.getRemainingTime(), 60);

    // 2. Turn off via "power" switch
    attrs["entity_id"] = "switch.bosch_lavavajillas_power_status";
    component.updateState(attrs, "off");

    // All should be reset to defaults
    EXPECT_FALSE(component.isExtraDry());
    EXPECT_FALSE(component.isHalfLoad());
    EXPECT_FALSE(component.isSpeedPerfect());
    EXPECT_EQ(component.getSelectedProgram(), "Dishcare.Dishwasher.Program.Eco50");
    EXPECT_EQ(component.getRemainingTime(), 0);

    // 3. Turn on features again, and test turning off via operation_state "inactive"
    attrs["entity_id"] = "switch.bosch_lavavajillas_extradry";
    component.updateState(attrs, "on");
    attrs["entity_id"] = "sensor.bosch_lavavajillas_selected_program";
    component.updateState(attrs, "Intensive");
    
    EXPECT_TRUE(component.isExtraDry());
    EXPECT_EQ(component.getSelectedProgram(), "Intensive");

    // Set operation_state to "inactive"
    attrs["entity_id"] = "sensor.bosch_lavavajillas_operation_state";
    component.updateState(attrs, "inactive");

    EXPECT_FALSE(component.isExtraDry());
    EXPECT_EQ(component.getSelectedProgram(), "Dishcare.Dishwasher.Program.Eco50");
}

TEST_F(DishwasherComponentTest, UpdateState_SelectedProgram) {
    QVariantMap mappings;
    mappings["selected_program"] = "sensor.bosch_lavavajillas_selected_program";
    component.setEntityMappings(mappings);

    QJsonObject attrs;
    attrs["entity_id"] = "sensor.bosch_lavavajillas_selected_program";
    
    component.updateState(attrs, "Eco 50°");
    EXPECT_EQ(component.getSelectedProgram(), "Eco 50°");
}

TEST_F(DishwasherComponentTest, ParseCommand_DefaultEntities) {
    QVariantMap mappings;
    mappings["power"] = "switch.bosch_lavavajillas_power_status";
    mappings["start"] = "button.bosch_lavavajillas_start";
    mappings["stop"] = "button.bosch_lavavajillas_stop";
    mappings["extradry"] = "switch.bosch_lavavajillas_extradry";
    mappings["half_load"] = "switch.bosch_lavavajillas_half_load";
    mappings["speedperfect"] = "switch.bosch_lavavajillas_speedperfect";
    component.setEntityMappings(mappings);

    // Test command ON default entity
    auto cmdOn = component.parseCommand("ON", controller);
    ASSERT_NE(cmdOn, nullptr);
    cmdOn->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_on");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_power_status");

    // Test command OFF default entity
    auto cmdOff = component.parseCommand("OFF", controller);
    ASSERT_NE(cmdOff, nullptr);
    cmdOff->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_off");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_power_status");

    // Test command START default entity
    auto cmdStart = component.parseCommand("START", controller);
    ASSERT_NE(cmdStart, nullptr);
    cmdStart->execute();
    EXPECT_EQ(controller.lastDomain, "button");
    EXPECT_EQ(controller.lastService, "press");
    EXPECT_EQ(controller.lastEntityId, "button.bosch_lavavajillas_start");

    // Test command STOP default entity
    auto cmdStop = component.parseCommand("STOP", controller);
    ASSERT_NE(cmdStop, nullptr);
    cmdStop->execute();
    EXPECT_EQ(controller.lastDomain, "button");
    EXPECT_EQ(controller.lastService, "press");
    EXPECT_EQ(controller.lastEntityId, "button.bosch_lavavajillas_stop");

    // Test command TOGGLE_EXTRADRY default entity (starts off, toggles on)
    auto cmdExtraDry = component.parseCommand("TOGGLE_EXTRADRY", controller);
    ASSERT_NE(cmdExtraDry, nullptr);
    cmdExtraDry->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_on");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_extradry");

    // Test command TOGGLE_HALFLOAD default entity (starts off, toggles on)
    auto cmdHalfLoad = component.parseCommand("TOGGLE_HALFLOAD", controller);
    ASSERT_NE(cmdHalfLoad, nullptr);
    cmdHalfLoad->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_on");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_half_load");

    // Test command TOGGLE_SPEEDPERFECT default entity (starts off, toggles on)
    auto cmdSpeedPerfect = component.parseCommand("TOGGLE_SPEEDPERFECT", controller);
    ASSERT_NE(cmdSpeedPerfect, nullptr);
    cmdSpeedPerfect->execute();
    EXPECT_EQ(controller.lastDomain, "switch");
    EXPECT_EQ(controller.lastService, "turn_on");
    EXPECT_EQ(controller.lastEntityId, "switch.bosch_lavavajillas_speedperfect");
}

TEST_F(DishwasherComponentTest, ParseCommand_DynamicDiscoveryAndAdvancedFeatures) {
    QJsonObject attrs;

    // 1. Discover power status switch dynamically
    QVariantMap mappings;
    mappings["power"] = "switch.custom_dishwasher_power";
    mappings["start"] = "button.custom_dishwasher_start";
    mappings["stop"] = "button.custom_dishwasher_stop";
    mappings["extradry"] = "switch.custom_dishwasher_extradry";
    mappings["half_load"] = "switch.custom_dishwasher_half_load";
    mappings["speedperfect"] = "switch.custom_dishwasher_speedperfect";
    component.setEntityMappings(mappings);

    attrs["entity_id"] = "switch.custom_dishwasher_power";
    component.updateState(attrs, "off");

    auto cmdOn = component.parseCommand("ON", controller);
    ASSERT_NE(cmdOn, nullptr);
    cmdOn->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_power");
    EXPECT_EQ(controller.lastService, "turn_on");

    auto cmdOff = component.parseCommand("OFF", controller);
    ASSERT_NE(cmdOff, nullptr);
    cmdOff->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_power");
    EXPECT_EQ(controller.lastService, "turn_off");

    // 2. Discover start button dynamically
    attrs["entity_id"] = "button.custom_dishwasher_start";
    component.updateState(attrs, "");

    auto cmdStart = component.parseCommand("START", controller);
    ASSERT_NE(cmdStart, nullptr);
    cmdStart->execute();
    EXPECT_EQ(controller.lastEntityId, "button.custom_dishwasher_start");
    EXPECT_EQ(controller.lastService, "press");

    // 3. Discover stop button dynamically
    attrs["entity_id"] = "button.custom_dishwasher_stop";
    component.updateState(attrs, "");

    auto cmdStop = component.parseCommand("STOP", controller);
    ASSERT_NE(cmdStop, nullptr);
    cmdStop->execute();
    EXPECT_EQ(controller.lastEntityId, "button.custom_dishwasher_stop");
    EXPECT_EQ(controller.lastService, "press");

    // 4. Discover ExtraDry switch dynamically and toggle logic
    attrs["entity_id"] = "switch.custom_dishwasher_extradry";
    component.updateState(attrs, "on"); // Sets state as true
    EXPECT_TRUE(component.isExtraDry());

    auto cmdExtraDryOff = component.parseCommand("TOGGLE_EXTRADRY", controller);
    ASSERT_NE(cmdExtraDryOff, nullptr);
    cmdExtraDryOff->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_extradry");
    EXPECT_EQ(controller.lastService, "turn_off");

    component.updateState(attrs, "off"); // Sets state as false
    EXPECT_FALSE(component.isExtraDry());

    auto cmdExtraDryOn = component.parseCommand("TOGGLE_EXTRADRY", controller);
    ASSERT_NE(cmdExtraDryOn, nullptr);
    cmdExtraDryOn->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_extradry");
    EXPECT_EQ(controller.lastService, "turn_on");

    // 5. Discover Half Load switch dynamically and toggle logic
    attrs["entity_id"] = "switch.custom_dishwasher_half_load";
    component.updateState(attrs, "on"); // Sets state as true
    EXPECT_TRUE(component.isHalfLoad());

    auto cmdHalfLoadOff = component.parseCommand("TOGGLE_HALFLOAD", controller);
    ASSERT_NE(cmdHalfLoadOff, nullptr);
    cmdHalfLoadOff->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_half_load");
    EXPECT_EQ(controller.lastService, "turn_off");

    component.updateState(attrs, "off"); // Sets state as false
    EXPECT_FALSE(component.isHalfLoad());

    auto cmdHalfLoadOn = component.parseCommand("TOGGLE_HALFLOAD", controller);
    ASSERT_NE(cmdHalfLoadOn, nullptr);
    cmdHalfLoadOn->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_half_load");
    EXPECT_EQ(controller.lastService, "turn_on");

    // 6. Discover SpeedPerfect switch dynamically and toggle logic
    attrs["entity_id"] = "switch.custom_dishwasher_speedperfect";
    component.updateState(attrs, "on"); // Sets state as true
    EXPECT_TRUE(component.isSpeedPerfect());

    auto cmdSpeedPerfectOff = component.parseCommand("TOGGLE_SPEEDPERFECT", controller);
    ASSERT_NE(cmdSpeedPerfectOff, nullptr);
    cmdSpeedPerfectOff->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_speedperfect");
    EXPECT_EQ(controller.lastService, "turn_off");

    component.updateState(attrs, "off"); // Sets state as false
    EXPECT_FALSE(component.isSpeedPerfect());

    auto cmdSpeedPerfectOn = component.parseCommand("TOGGLE_SPEEDPERFECT", controller);
    ASSERT_NE(cmdSpeedPerfectOn, nullptr);
    cmdSpeedPerfectOn->execute();
    EXPECT_EQ(controller.lastEntityId, "switch.custom_dishwasher_speedperfect");
    EXPECT_EQ(controller.lastService, "turn_on");
}

TEST_F(DishwasherComponentTest, SensorBridgeIntegration) {
    DeviceModel model;
    MockSettingsManager settings;
    DeviceFactory factory;

    factory.registerType("Dishwasher", [](const std::string& id, const std::string& topic) {
        auto dev = std::make_unique<HomeDeviceBase>(id, topic, DeviceType::Dishwasher);
        dev->addComponent(std::make_unique<SwitchableComponent>(dev.get()));
        dev->addComponent(std::make_unique<DishwasherComponent>(dev.get()));
        return dev;
    });

    MockHaController haController;
    MockLicenseManager mockLicense;
    SensorBridge bridge(factory, model, haController, settings, mockLicense);

    QJsonObject attrs;
    
    // Simulate discovering the door
    attrs["entity_id"] = "binary_sensor.bosch_405030540024000050_bsh_common_status_doorstate";
    attrs["friendly_name"] = "Door";
    bridge.onDeviceDiscovered("binary_sensor", "binary_sensor.bosch_405030540024000050_bsh_common_status_doorstate", "Door", "off", attrs);

    // Simulate discovering the operation state
    attrs["entity_id"] = "sensor.bosch_405030540024000050_bsh_common_status_operationstate";
    attrs["friendly_name"] = "Operation State";
    bridge.onDeviceDiscovered("sensor", "sensor.bosch_405030540024000050_bsh_common_status_operationstate", "Operation State", "Run", attrs);

    // Simulate discovering the remaining time
    attrs["entity_id"] = "sensor.bosch_405030540024000050_bsh_common_option_remainingprogramtime";
    attrs["friendly_name"] = "Remaining Time";
    bridge.onDeviceDiscovered("sensor", "sensor.bosch_405030540024000050_bsh_common_option_remainingprogramtime", "Remaining Time", "7200", attrs);

    // We should have a unified dishwasher device named dishwasher.bosch_lavavajillas
    auto dwDevice = model.findByTopic("dishwasher.bosch_lavavajillas");
    
    ASSERT_NE(dwDevice, nullptr);

    EXPECT_TRUE(dwDevice->hasComponent("dishwasher"));
    auto dwComponent = dwDevice->getComponent("dishwasher");
    ASSERT_NE(dwComponent, nullptr);
    EXPECT_EQ(dwComponent->getProperty("dishwasherRemainingTime").toInt(), 120);
    EXPECT_EQ(dwComponent->getProperty("dishwasherDoorOpen").toBool(), false);
}

// Unit Tests for DeviceMappingManager
TEST(DeviceMappingManagerTest, ParseValidJson) {
    DeviceMappingManager manager;
    
    QJsonObject entities;
    entities["power"] = "switch.dishwasher_power";
    entities["door"] = "binary_sensor.dishwasher_door";
    
    QJsonObject device;
    device["id"] = "dishwasher.bosch";
    device["name"] = "Bosch Dishwasher";
    device["type"] = "Dishwasher";
    device["entities"] = entities;
    
    QJsonArray compoundDevices;
    compoundDevices.append(device);
    
    QJsonObject root;
    root["compound_devices"] = compoundDevices;
    
    manager.loadFromJsonObject(root);
    
    EXPECT_TRUE(manager.isEntityMapped("switch.dishwasher_power"));
    EXPECT_TRUE(manager.isEntityMapped("SWITCH.DISHWASHER_POWER")); // case insensitivity check
    EXPECT_TRUE(manager.isEntityMapped("binary_sensor.dishwasher_door"));
    EXPECT_FALSE(manager.isEntityMapped("switch.some_other_entity"));
    
    EXPECT_EQ(manager.getDeviceTopicForEntity("switch.dishwasher_power"), "dishwasher.bosch");
    EXPECT_EQ(manager.getDeviceTypeForEntity("switch.dishwasher_power"), "Dishwasher");
    EXPECT_EQ(manager.getDeviceNameForEntity("switch.dishwasher_power"), "Bosch Dishwasher");
    
    QVariantMap mappings = manager.getEntityMappingsForDevice("dishwasher.bosch");
    EXPECT_EQ(mappings.value("power").toString(), "switch.dishwasher_power");
    EXPECT_EQ(mappings.value("door").toString(), "binary_sensor.dishwasher_door");
}

TEST(DeviceMappingManagerTest, ParseInvalidJsonMissingId) {
    DeviceMappingManager manager;
    
    QJsonObject device;
    device["name"] = "Invalid Device";
    device["type"] = "Dishwasher";
    
    QJsonArray compoundDevices;
    compoundDevices.append(device);
    
    QJsonObject root;
    root["compound_devices"] = compoundDevices;
    
    manager.loadFromJsonObject(root);
    EXPECT_FALSE(manager.isEntityMapped("switch.dishwasher_power"));
}

TEST(DeviceMappingManagerTest, LoadFromFile) {
    // Write a temporary config JSON file
    QString tempFilename = "temp_test_config.json";
    QFile file(tempFilename);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    
    QByteArray jsonContent = R"({
        "compound_devices": [
            {
                "id": "dishwasher.temp",
                "name": "Temp Dishwasher",
                "type": "Dishwasher",
                "entities": {
                    "power": "switch.temp_power"
                }
            }
        ]
    })";
    
    file.write(jsonContent);
    file.close();
    
    DeviceMappingManager manager;
    EXPECT_TRUE(manager.loadMappings(tempFilename));
    EXPECT_TRUE(manager.isEntityMapped("switch.temp_power"));
    EXPECT_EQ(manager.getDeviceTopicForEntity("switch.temp_power"), "dishwasher.temp");
    
    // Clean up
    QFile::remove(tempFilename);
}
