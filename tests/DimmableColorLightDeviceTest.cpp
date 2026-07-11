#include <gtest/gtest.h>
#include "Devices/DimmableColorLightDevice.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

TEST(DimmableColorLightDeviceTest, InitialState) {
    DimmableColorLightDevice light("test_id", "light.test_id");
    EXPECT_FALSE(light.isOn());
    EXPECT_FLOAT_EQ(light.getValue(), 0.0f);
    EXPECT_EQ(light.getColor(), "#FFFFFF");
}

TEST(DimmableColorLightDeviceTest, UpdateState_OnOff) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    DimmableColorLightDevice light("test_id", "light.test_id");
    bool updatedEmitted = false;
    QObject::connect(&light, &HomeDeviceBase::updated, [&]() {
        updatedEmitted = true;
    });

    light.updateState("on", {});
    QCoreApplication::processEvents();
    EXPECT_TRUE(light.isOn());
    EXPECT_TRUE(updatedEmitted);

    updatedEmitted = false;
    light.updateState("off", {});
    QCoreApplication::processEvents();
    EXPECT_FALSE(light.isOn());
    EXPECT_TRUE(updatedEmitted);
}

TEST(DimmableColorLightDeviceTest, UpdateState_Brightness) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    DimmableColorLightDevice light("test_id", "light.test_id");
    bool updatedEmitted = false;
    QObject::connect(&light, &HomeDeviceBase::updated, [&]() {
        updatedEmitted = true;
    });

    QJsonObject attributes;
    attributes["brightness"] = 127.0;

    light.updateState("on", attributes);
    QCoreApplication::processEvents();
    EXPECT_NEAR(light.getValue(), 127.0f / 255.0f, 0.001f);
    EXPECT_TRUE(updatedEmitted);
}

TEST(DimmableColorLightDeviceTest, UpdateState_ColorRGB) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    DimmableColorLightDevice light("test_id", "light.test_id");
    bool updatedEmitted = false;
    QObject::connect(&light, &HomeDeviceBase::updated, [&]() {
        updatedEmitted = true;
    });

    QJsonObject attributes;
    QJsonArray rgb;
    rgb.append(255);
    rgb.append(0);
    rgb.append(255);
    attributes["rgb_color"] = rgb;

    light.updateState("on", attributes);
    QCoreApplication::processEvents();
    EXPECT_EQ(light.getColor(), "#FF00FF");
    EXPECT_TRUE(updatedEmitted);
}

TEST(DimmableColorLightDeviceTest, PrepareForCommand_Directives) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    DimmableColorLightDevice light("test_id", "light.test_id");

    light.prepareForCommand("ON");
    EXPECT_TRUE(light.isOn());

    light.prepareForCommand("OFF");
    EXPECT_FALSE(light.isOn());

    light.prepareForCommand("BRIGHTNESS:85");
    EXPECT_TRUE(light.isOn());
    EXPECT_FLOAT_EQ(light.getValue(), 0.85f);

    light.prepareForCommand("COLOR:#FFAA00");
    EXPECT_TRUE(light.isOn());
    EXPECT_EQ(light.getColor(), "#FFAA00");
}
