#include <gtest/gtest.h>
#include "Devices/HomeDeviceBase.h"
#include "Devices/SwitchableComponent.h"
#include "Devices/DimmableComponent.h"
#include "Devices/ColorableComponent.h"
#include "Devices/StoppableComponent.h"
#include "Devices/StatusComponent.h"
#include "Network/IHaController.h"
#include "Commands/ICommand.h"

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
