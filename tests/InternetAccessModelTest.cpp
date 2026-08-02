#include <gtest/gtest.h>
#include <map>
#include <string>
#include <QJsonObject>
#include <QCoreApplication>
#include "Devices/InternetAccessModel.h"
#include "SensorBridge.h"
#include "HomeDeviceBase.h"
#include "Network/IHaController.h"
#include "IDeviceFactory.h"
#include "DeviceModel.h"
#include "ISettingsManager.h"
#include "Devices/ILicenseManager.h"

namespace {

class MockLicenseManager : public ILicenseManager {
public:
    bool isFeatureLicensed(const std::string& featureId) const override { return true; }
    bool activateFeature(const std::string& featureId, const std::string& licenseKey) override { return true; }
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

class MockDeviceFactory : public IDeviceFactory {
public:
    std::unique_ptr<HomeDeviceBase> create(const std::string& type, const std::string& id, const std::string& topic) override {
        return nullptr;
    }
    void registerType(const std::string& type, std::function<std::unique_ptr<HomeDeviceBase>(const std::string&, const std::string&)> creator) override {}
};

} // namespace

TEST(InternetAccessModelTest, InitialState) {
    InternetAccessModel model;
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.activeCount(), 0);

    QHash<int, QByteArray> roles = model.roleNames();
    EXPECT_TRUE(roles.contains(InternetAccessModel::NameRole));
    EXPECT_TRUE(roles.contains(InternetAccessModel::EntityIdRole));
    EXPECT_TRUE(roles.contains(InternetAccessModel::TypeRole));
    EXPECT_TRUE(roles.contains(InternetAccessModel::IsActiveRole));
    EXPECT_EQ(roles[InternetAccessModel::NameRole], "name");
    EXPECT_EQ(roles[InternetAccessModel::EntityIdRole], "entityId");
    EXPECT_EQ(roles[InternetAccessModel::TypeRole], "type");
    EXPECT_EQ(roles[InternetAccessModel::IsActiveRole], "isActive");
}

TEST(InternetAccessModelTest, NoiseFilterDomoticsAndAppliancesIgnored) {
    InternetAccessModel model;

    // Domotics and appliances keywords: washer, vacuum, dishwasher, light, cover, dimmer, sensor
    QVector<QPair<QString, QString>> ignoredDevices = {
        {"switch.washer_kitchen", "Kitchen Washer"},
        {"switch.vacuum_robot", "Robot Vacuum"},
        {"switch.dishwasher_main", "Dishwasher"},
        {"light.living_room", "Living Room Light"},
        {"cover.bedroom_window", "Bedroom Cover"},
        {"switch.dimmer_hall", "Hall Dimmer"},
        {"sensor.temperature_sensor", "Temperature Sensor"}
    };

    for (const auto& dev : ignoredDevices) {
        model.handleTrackerUpdate("device_tracker." + QString(dev.first).replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", dev.second);
        model.handleSwitchUpdate(dev.first, "on", dev.second);
        EXPECT_EQ(model.rowCount(), 0) << "Device should be filtered out: " << dev.first.toStdString();
    }
}

TEST(InternetAccessModelTest, NoiseFilterEntertainmentAccepted) {
    InternetAccessModel model;

    // Entertainment devices keywords: tv, television, phone, mobile, playstation, xbox, nintendo, switch, pc, computer, ipad, tablet, macbook
    QVector<QPair<QString, QString>> acceptedDevices = {
        {"tv_living_room", "Living Room TV"},
        {"my_television", "Main Television"},
        {"phone_john", "John's Phone"},
        {"mobile_mom", "Mom's Mobile"},
        {"playstation_console", "Playstation 5"},
        {"xbox_series", "Xbox Console"},
        {"nintendo_switch", "Kids Nintendo"},
        {"nintendo_game_console", "nintendo switch"},
        {"pc_office", "Office PC"},
        {"computer_main", "Main Computer"},
        {"ipad_kid", "Kid's iPad"},
        {"tablet_mom", "Mom's Tablet"},
        {"macbook_work", "Work MacBook"}
    };

    int count = 0;
    for (const auto& dev : acceptedDevices) {
        model.handleTrackerUpdate("device_tracker." + QString(dev.first).replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", dev.second);
        model.handleSwitchUpdate(dev.first, "on", dev.second);
        count++;
        EXPECT_EQ(model.rowCount(), count) << "Device should be accepted: " << dev.first.toStdString();
    }
}

TEST(InternetAccessModelTest, TypeClassification) {
    // Television 1 (contains "tv" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("tv_device").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "My Smart Device");
        model.handleSwitchUpdate("tv_device", "on", "My Smart Device");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "television");
    }

    // Television 2 (contains "television" in friendlyName)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("living_room_device").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room Television");
        model.handleSwitchUpdate("living_room_device", "on", "Living Room Television");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "television");
    }

    // Smartphone 1 (contains "phone" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("iphone_john").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "John's Device");
        model.handleSwitchUpdate("iphone_john", "on", "John's Device");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "smartphone");
    }

    // Smartphone 2 (contains "mobile" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("device_mobile").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Work Device");
        model.handleSwitchUpdate("device_mobile", "on", "Work Device");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "smartphone");
    }

    // Smartphone 3 (contains "ipad" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("ipad_pro").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "iPad Pro");
        model.handleSwitchUpdate("ipad_pro", "on", "iPad Pro");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "smartphone");
    }

    // Smartphone 4 (contains "tablet" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("tablet_device").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Android Tablet");
        model.handleSwitchUpdate("tablet_device", "on", "Android Tablet");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "smartphone");
    }

    // Console 1 (contains "playstation" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("playstation_5").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "PS5");
        model.handleSwitchUpdate("playstation_5", "on", "PS5");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "console");
    }

    // Console 2 (contains "xbox" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("xbox_one").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Xbox");
        model.handleSwitchUpdate("xbox_one", "on", "Xbox");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "console");
    }

    // Console 3 (contains "switch" in entityId)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("nintendo_switch").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Switch");
        model.handleSwitchUpdate("nintendo_switch", "on", "Switch");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "console");
    }

    // Generic 1 (contains "pc" in entityId, does not match tv/phone/switch)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("pc_gaming").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Gaming Machine");
        model.handleSwitchUpdate("pc_gaming", "on", "Gaming Machine");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "generic");
    }

    // Generic 2 (contains "computer" in entityId, does not match tv/phone/switch)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("office_computer").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Office iMac");
        model.handleSwitchUpdate("office_computer", "on", "Office iMac");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "generic");
    }

    // Generic 3 (contains "macbook" in entityId, does not match tv/phone/switch)
    {
        InternetAccessModel model;
        model.handleTrackerUpdate("device_tracker." + QString("work_macbook").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Work MacBook Pro");
        model.handleSwitchUpdate("work_macbook", "on", "Work MacBook Pro");
        ASSERT_EQ(model.rowCount(), 1);
        EXPECT_EQ(model.data(model.index(0), InternetAccessModel::TypeRole).toString(), "generic");
    }
}

TEST(InternetAccessModelTest, InsertionAndStateUpdates) {
    InternetAccessModel model;

    // Test initial insertion with state "on" -> isActive = true
    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "on", "Living Room TV");
    EXPECT_EQ(model.rowCount(), 1);
    QModelIndex idx = model.index(0);
    EXPECT_TRUE(model.data(idx, InternetAccessModel::IsActiveRole).toBool());

    // Test initial insertion with state "off" -> isActive = false
    model.handleTrackerUpdate("device_tracker." + QString("playstation_console").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Playstation");
    model.handleSwitchUpdate("playstation_console", "off", "Playstation");
    EXPECT_EQ(model.rowCount(), 2);
    QModelIndex idx2 = model.index(1);
    EXPECT_FALSE(model.data(idx2, InternetAccessModel::IsActiveRole).toBool());

    // Test transition from on to off
    bool dataChangedEmitted = false;
    int changedRow = -1;
    QList<int> changedRoles;
    QObject::connect(&model, &InternetAccessModel::dataChanged, [&](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
        dataChangedEmitted = true;
        changedRow = topLeft.row();
        changedRoles = roles.toList();
    });

    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "off", "Living Room TV");
    EXPECT_TRUE(dataChangedEmitted);
    EXPECT_EQ(changedRow, 0);
    EXPECT_TRUE(changedRoles.contains(InternetAccessModel::IsActiveRole));
    EXPECT_FALSE(model.data(idx, InternetAccessModel::IsActiveRole).toBool());

    // Reset signal tracker and test transition from off to on
    dataChangedEmitted = false;
    changedRow = -1;
    changedRoles.clear();

    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "on", "Living Room TV");
    EXPECT_TRUE(dataChangedEmitted);
    EXPECT_EQ(changedRow, 0);
    EXPECT_TRUE(changedRoles.contains(InternetAccessModel::IsActiveRole));
    EXPECT_TRUE(model.data(idx, InternetAccessModel::IsActiveRole).toBool());

    // Reset signal tracker and test that updating with the same state does NOT trigger dataChanged
    dataChangedEmitted = false;
    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "on", "Living Room TV");
    EXPECT_FALSE(dataChangedEmitted);
}

TEST(InternetAccessModelTest, DynamicCount) {
    InternetAccessModel model;
    EXPECT_EQ(model.activeCount(), 0);

    // Add first accepted device
    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "on", "Living Room TV");
    EXPECT_EQ(model.activeCount(), 1);

    // Add ignored device (should not change count)
    model.handleTrackerUpdate("device_tracker." + QString("switch.washer_kitchen").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Kitchen Washer");
    model.handleSwitchUpdate("switch.washer_kitchen", "on", "Kitchen Washer");
    EXPECT_EQ(model.activeCount(), 1);

    // Add second accepted device (off state)
    model.handleTrackerUpdate("device_tracker." + QString("playstation_console").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Playstation");
    model.handleSwitchUpdate("playstation_console", "off", "Playstation");
    EXPECT_EQ(model.activeCount(), 2);

    // Add duplicate device (should not change count)
    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "on", "Living Room TV");
    EXPECT_EQ(model.activeCount(), 2);

    // Transition state (should not change count of total elements in model since it's size of m_devices)
    model.handleTrackerUpdate("device_tracker." + QString("tv_living_room").replace("switch.", "").replace("_acceso_a_internet", "").replace("_internet_access", "").replace("_internet", ""), "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "off", "Living Room TV");
    EXPECT_EQ(model.activeCount(), 2);
}

TEST(InternetAccessModelTest, HotSyncPresence) {
    InternetAccessModel model;

    // Initial state: switch is added but tracker is offline
    model.handleSwitchUpdate("switch.damogi_pc_acceso_a_internet", "on", "Damogi PC Internet");
    EXPECT_EQ(model.rowCount(), 0) << "Should be 0 because tracker is offline";

    // Tracker goes home
    model.handleTrackerUpdate("device_tracker.damogi_pc", "home", "Damogi PC");
    EXPECT_EQ(model.rowCount(), 1) << "Should be 1 after tracker goes home";

    // Tracker goes away
    model.handleTrackerUpdate("device_tracker.damogi_pc", "not_home", "Damogi PC");
    EXPECT_EQ(model.rowCount(), 0) << "Should be 0 after tracker goes away";
}

TEST(InternetAccessModelTest, NormalizationLogic) {
    InternetAccessModel model;
    
    model.handleTrackerUpdate("device_tracker.damogi_pc", "home", "Damogi PC");
    model.handleSwitchUpdate("switch.damogi_pc_acceso_a_internet", "on", "Damogi PC Internet");
    
    EXPECT_EQ(model.rowCount(), 1);
    QModelIndex idx = model.index(0);
    EXPECT_EQ(model.data(idx, InternetAccessModel::EntityIdRole).toString().toStdString(), "switch.damogi_pc_acceso_a_internet");
}

TEST(InternetAccessModelTest, IntegrationWithSensorBridge) {
    MockDeviceFactory mockFactory;
    DeviceModel deviceModel;
    MockHaController mockHa;
    MockSettingsManager mockSettings;
    MockLicenseManager mockLicense;
    SensorBridge bridge{mockFactory, deviceModel, mockHa, mockSettings, mockLicense};

    // Toggle Internet ON
    bridge.toggleInternet("switch.playstation_internet_access", true);
    EXPECT_EQ(mockHa.lastDomain, "switch");
    EXPECT_EQ(mockHa.lastService, "turn_on");
    EXPECT_EQ(mockHa.lastEntityId, "switch.playstation_internet_access");

    // Toggle Internet OFF
    bridge.toggleInternet("switch.playstation_internet_access", false);
    EXPECT_EQ(mockHa.lastDomain, "switch");
    EXPECT_EQ(mockHa.lastService, "turn_off");
    EXPECT_EQ(mockHa.lastEntityId, "switch.playstation_internet_access");
}

TEST(InternetAccessModelTest, ClearModel) {
    InternetAccessModel model;
    
    // Add an accepted device
    model.handleTrackerUpdate("device_tracker.tv_living_room", "home", "Living Room TV");
    model.handleSwitchUpdate("tv_living_room", "on", "Living Room TV");
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.activeCount(), 1);

    // Clear the model
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.activeCount(), 0);
}
