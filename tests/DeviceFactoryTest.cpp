#include <gtest/gtest.h>
#include "Devices/DeviceFactory.h"
#include "Devices/HomeDeviceBase.h"

class DummyDevice : public HomeDeviceBase {
public:
    DummyDevice(const std::string& id, const std::string& topic)
        : HomeDeviceBase(id, topic) {}

    DeviceType getType() const override { return DeviceType::Light; }
    void prepareForCommand(const std::string& payload) override {}
    void updateState(const std::string& state, const QJsonObject& attributes) override {}
};

class AlternativeDummyDevice : public HomeDeviceBase {
public:
    AlternativeDummyDevice(const std::string& id, const std::string& topic)
        : HomeDeviceBase(id, topic) {}

    DeviceType getType() const override { return DeviceType::Vacuum; }
    void prepareForCommand(const std::string& payload) override {}
    void updateState(const std::string& state, const QJsonObject& attributes) override {}
};

TEST(DeviceFactoryTest, RegisterAndCreate) {
    DeviceFactory factory;
    factory.registerType("dummy", [](const std::string& id, const std::string& topic) {
        return std::make_unique<DummyDevice>(id, topic);
    });

    auto device = factory.create("dummy", "id123", "topic123");
    ASSERT_NE(device, nullptr);
    EXPECT_EQ(device->getId(), "id123");
    EXPECT_EQ(device->getQStringTopic().toStdString(), "topic123");
}

TEST(DeviceFactoryTest, UnregisteredType) {
    DeviceFactory factory;
    auto device = factory.create("nonexistent", "id123", "topic123");
    EXPECT_EQ(device, nullptr);
}

TEST(DeviceFactoryTest, OverwriteRegistration) {
    DeviceFactory factory;
    factory.registerType("dummy", [](const std::string& id, const std::string& topic) {
        return std::make_unique<DummyDevice>(id, topic);
    });

    factory.registerType("dummy", [](const std::string& id, const std::string& topic) {
        return std::make_unique<AlternativeDummyDevice>(id, topic);
    });

    auto device = factory.create("dummy", "id123", "topic123");
    ASSERT_NE(device, nullptr);
    auto* altPtr = dynamic_cast<AlternativeDummyDevice*>(device.get());
    EXPECT_NE(altPtr, nullptr);
}
