#include <gtest/gtest.h>
#include <QJsonObject>
#include <QJsonArray>
#include "Devices/HomeDeviceBase.h"
#include "Devices/SwitchableComponent.h"
#include "Devices/DimmableComponent.h"
#include "Devices/ColorableComponent.h"
#include "Devices/StoppableComponent.h"
#include "Devices/StatusComponent.h"
#include "Network/IHaController.h"
#include "Commands/ICommand.h"
#include "Commands/GenericHaCommand.h"

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

TEST(DeviceComponentTest, SwitchableLifecycle) {
    auto dev = std::make_unique<HomeDeviceBase>("plug1", "switch.plug1", DeviceType::Light);
    dev->addComponent(std::make_unique<SwitchableComponent>(dev.get()));
    
    EXPECT_TRUE(dev->hasComponent("switchable"));
    auto* switchable = static_cast<SwitchableComponent*>(dev->getComponent("switchable"));
    ASSERT_NE(switchable, nullptr);
    EXPECT_FALSE(switchable->isOn());

    dev->updateState("on", {});
    EXPECT_TRUE(switchable->isOn());

    MockHaController mockHa;
    auto cmd = dev->parseCommand("OFF", mockHa);
    ASSERT_NE(cmd, nullptr);
    cmd->execute();
    EXPECT_EQ(mockHa.lastService, "turn_off");
}

TEST(DeviceComponentTest, DimmableLifecycle) {
    auto dev = std::make_unique<HomeDeviceBase>("light1", "light.light1", DeviceType::Light);
    dev->addComponent(std::make_unique<DimmableComponent>(dev.get()));
    
    auto* dimmable = static_cast<DimmableComponent*>(dev->getComponent("dimmable"));
    ASSERT_NE(dimmable, nullptr);
    
    QJsonObject attrs;
    attrs["brightness"] = 127;
    dev->updateState("on", attrs);
    EXPECT_NEAR(dimmable->getLevel(), 127.0f / 255.0f, 0.01f);

    MockHaController mockHa;
    auto cmd = dev->parseCommand("BRIGHTNESS:50", mockHa);
    ASSERT_NE(cmd, nullptr);
    cmd->execute();
    EXPECT_EQ(mockHa.lastService, "turn_on");
    EXPECT_EQ(mockHa.lastServiceData["brightness"].toInt(), 127); // 50% of 255
}

TEST(DeviceComponentTest, ColorableLifecycle) {
    auto dev = std::make_unique<HomeDeviceBase>("light2", "light.color_light", DeviceType::Light);
    dev->addComponent(std::make_unique<ColorableComponent>(dev.get()));

    EXPECT_TRUE(dev->hasComponent("colorable"));
    auto* colorable = static_cast<ColorableComponent*>(dev->getComponent("colorable"));
    ASSERT_NE(colorable, nullptr);

    // Initial values
    EXPECT_EQ(colorable->getColor(), "#FFFFFF");
    EXPECT_FALSE(colorable->supportsColorTemp());
    EXPECT_EQ(colorable->getColorTemp(), 4000);
    EXPECT_EQ(colorable->getMinColorTemp(), 2000);
    EXPECT_EQ(colorable->getMaxColorTemp(), 6500);

    // Update state with Kelvin temp config
    QJsonObject attrs;
    attrs["min_color_temp_kelvin"] = 2200;
    attrs["max_color_temp_kelvin"] = 6000;
    attrs["color_temp_kelvin"] = 3000;
    
    QJsonArray modes;
    modes.append("color_temp");
    attrs["supported_color_modes"] = modes;
    attrs["color_mode"] = "color_temp";

    dev->updateState("on", attrs);
    EXPECT_EQ(colorable->getMinColorTemp(), 2200);
    EXPECT_EQ(colorable->getMaxColorTemp(), 6000);
    EXPECT_EQ(colorable->getColorTemp(), 3000);
    EXPECT_TRUE(colorable->supportsColorTemp());
    EXPECT_EQ(colorable->getColor(), "#FFFFFF");

    // Update with RGB color
    QJsonObject rgbAttrs;
    QJsonArray rgbArray;
    rgbArray.append(255);
    rgbArray.append(128);
    rgbArray.append(0);
    rgbAttrs["rgb_color"] = rgbArray;
    rgbAttrs["color_mode"] = "rgb";

    dev->updateState("on", rgbAttrs);
    EXPECT_EQ(colorable->getColor(), "#FF8000");

    // Test parseCommand KELVIN
    MockHaController mockHa;
    auto cmdKelvin = dev->parseCommand("KELVIN:3200", mockHa);
    ASSERT_NE(cmdKelvin, nullptr);
    cmdKelvin->execute();
    EXPECT_EQ(mockHa.lastDomain, "light");
    EXPECT_EQ(mockHa.lastService, "turn_on");
    EXPECT_EQ(mockHa.lastEntityId, "light.color_light");
    EXPECT_EQ(mockHa.lastServiceData["color_temp_kelvin"].toInt(), 3200);

    // Test parseCommand COLOR #FFFFFF (Cold white limit)
    auto cmdWhite = dev->parseCommand("COLOR:#FFFFFF", mockHa);
    ASSERT_NE(cmdWhite, nullptr);
    cmdWhite->execute();
    EXPECT_EQ(mockHa.lastServiceData["color_temp_kelvin"].toInt(), 6000); // max temperature set earlier

    // Test parseCommand COLOR #FF0000 (Red)
    auto cmdRed = dev->parseCommand("COLOR:#FF0000", mockHa);
    ASSERT_NE(cmdRed, nullptr);
    cmdRed->execute();
    QJsonArray expectedRgb = mockHa.lastServiceData["rgb_color"].toArray();
    ASSERT_EQ(expectedRgb.size(), 3);
    EXPECT_EQ(expectedRgb[0].toInt(), 255);
    EXPECT_EQ(expectedRgb[1].toInt(), 0);
    EXPECT_EQ(expectedRgb[2].toInt(), 0);

    // Test prepareForCommand
    colorable->prepareForCommand("COLOR:#00FF00");
    EXPECT_EQ(colorable->getColor(), "#00FF00");

    colorable->prepareForCommand("KELVIN:4500");
    EXPECT_EQ(colorable->getColorTemp(), 4500);
    EXPECT_EQ(colorable->getColor(), "#FFFFFF");
}

TEST(DeviceComponentTest, StoppableLifecycle) {
    auto dev = std::make_unique<HomeDeviceBase>("blind1", "cover.living_blind", DeviceType::Roller);
    dev->addComponent(std::make_unique<StoppableComponent>(dev.get()));

    EXPECT_TRUE(dev->hasComponent("stoppable"));
    auto* stoppable = static_cast<StoppableComponent*>(dev->getComponent("stoppable"));
    ASSERT_NE(stoppable, nullptr);

    // Initial state
    EXPECT_FALSE(stoppable->isMoving());

    // Update states
    dev->updateState("opening", {});
    EXPECT_TRUE(stoppable->isMoving());

    dev->updateState("closing", {});
    EXPECT_TRUE(stoppable->isMoving());

    dev->updateState("open", {});
    EXPECT_FALSE(stoppable->isMoving());

    // Commands
    MockHaController mockHa;
    auto cmdStop = dev->parseCommand("STOP", mockHa);
    ASSERT_NE(cmdStop, nullptr);
    cmdStop->execute();
    EXPECT_EQ(mockHa.lastDomain, "cover");
    EXPECT_EQ(mockHa.lastService, "stop_cover");
    EXPECT_EQ(mockHa.lastEntityId, "cover.living_blind");

    // prepareForCommand
    stoppable->prepareForCommand("STOP");
    EXPECT_FALSE(stoppable->isMoving());

    stoppable->prepareForCommand("UP");
    EXPECT_TRUE(stoppable->isMoving());
}

TEST(DeviceComponentTest, StatusLifecycle) {
    auto dev = std::make_unique<HomeDeviceBase>("vacuum1", "vacuum.robot", DeviceType::Vacuum);
    dev->addComponent(std::make_unique<StatusComponent>(dev.get()));

    EXPECT_TRUE(dev->hasComponent("status"));
    auto* status = static_cast<StatusComponent*>(dev->getComponent("status"));
    ASSERT_NE(status, nullptr);

    // Initial state
    EXPECT_EQ(status->getStatusState(), "");
    EXPECT_EQ(status->getBatteryLevel(), 0);
    EXPECT_EQ(status->getFanSpeed(), "");

    // Update state
    QJsonObject attrs;
    attrs["battery_level"] = 85;
    attrs["fan_speed"] = "high";
    dev->updateState("cleaning", attrs);

    EXPECT_EQ(status->getStatusState(), "cleaning");
    EXPECT_EQ(status->getBatteryLevel(), 85);
    EXPECT_EQ(status->getFanSpeed(), "high");

    // Commands
    MockHaController mockHa;
    auto cmdLocate = dev->parseCommand("LOCATE", mockHa);
    ASSERT_NE(cmdLocate, nullptr);
    cmdLocate->execute();
    EXPECT_EQ(mockHa.lastDomain, "vacuum");
    EXPECT_EQ(mockHa.lastService, "locate");

    auto cmdFan = dev->parseCommand("FAN_SPEED:medium", mockHa);
    ASSERT_NE(cmdFan, nullptr);
    cmdFan->execute();
    EXPECT_EQ(mockHa.lastService, "set_fan_speed");
    EXPECT_EQ(mockHa.lastServiceData["fan_speed"].toString(), "medium");

    auto cmdCustom = dev->parseCommand("SEND_COMMAND:clean_spot", mockHa);
    ASSERT_NE(cmdCustom, nullptr);
    cmdCustom->execute();
    EXPECT_EQ(mockHa.lastService, "send_command");
    EXPECT_EQ(mockHa.lastServiceData["command"].toString(), "clean_spot");

    // prepareForCommand
    status->prepareForCommand("START");
    EXPECT_EQ(status->getStatusState(), "cleaning");

    status->prepareForCommand("PAUSE");
    EXPECT_EQ(status->getStatusState(), "paused");

    status->prepareForCommand("RETURN");
    EXPECT_EQ(status->getStatusState(), "returning");
}
