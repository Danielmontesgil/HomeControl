#include <gtest/gtest.h>
#include "Devices/DeviceModel.h"
#include "Devices/DimmableColorLightDevice.h"
#include <QCoreApplication>

TEST(DeviceModelTest, InitialEmpty) {
    DeviceModel model;
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(DeviceModelTest, AddDeviceAndRowCount) {
    DeviceModel model;

    bool aboutToBeInsertedEmitted = false;
    bool insertedEmitted = false;

    QObject::connect(&model, &DeviceModel::rowsAboutToBeInserted, [&](const QModelIndex& parent, int first, int last) {
        aboutToBeInsertedEmitted = true;
        EXPECT_EQ(first, 0);
        EXPECT_EQ(last, 0);
    });
    QObject::connect(&model, &DeviceModel::rowsInserted, [&](const QModelIndex& parent, int first, int last) {
        insertedEmitted = true;
        EXPECT_EQ(first, 0);
        EXPECT_EQ(last, 0);
    });

    model.addDevice(std::make_unique<DimmableColorLightDevice>("light1", "light.light1"));
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_TRUE(aboutToBeInsertedEmitted);
    EXPECT_TRUE(insertedEmitted);
}

TEST(DeviceModelTest, RoleMapping) {
    DeviceModel model;
    auto light = std::make_unique<DimmableColorLightDevice>("light1", "light.light1");
    model.addDevice(std::move(light));

    QModelIndex index = model.index(0);

    EXPECT_EQ(model.data(index, DeviceModel::IdRole).toString(), "light1");
    EXPECT_EQ(model.data(index, DeviceModel::TopicRole).toString(), "light.light1");
    EXPECT_FLOAT_EQ(model.data(index, DeviceModel::ValueRole).toFloat(), 0.0f);
    EXPECT_FALSE(model.data(index, DeviceModel::IsOnRole).toBool());
    EXPECT_TRUE(model.data(index, DeviceModel::SupportsColorRole).toBool());
    EXPECT_EQ(model.data(index, DeviceModel::ColorRole).toString(), "#FFFFFF");
    EXPECT_EQ(model.data(index, DeviceModel::TypeRole).toInt(), static_cast<int>(DeviceType::Light));
}

TEST(DeviceModelTest, DataChangedSignal) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    DeviceModel model;
    auto light = std::make_unique<DimmableColorLightDevice>("light1", "light.light1");
    auto* lightPtr = light.get();
    model.addDevice(std::move(light));

    bool dataChangedEmitted = false;
    QObject::connect(&model, &DeviceModel::dataChanged, [&](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
        dataChangedEmitted = true;
        EXPECT_EQ(topLeft.row(), 0);
        EXPECT_EQ(bottomRight.row(), 0);
    });

    lightPtr->prepareForCommand("BRIGHTNESS:85");
    QCoreApplication::processEvents();

    EXPECT_TRUE(dataChangedEmitted);
}

TEST(DeviceModelTest, FindByTopic) {
    DeviceModel model;
    model.addDevice(std::make_unique<DimmableColorLightDevice>("light1", "light.light1"));
    model.addDevice(std::make_unique<DimmableColorLightDevice>("light2", "light.light2"));

    auto* dev = model.findByTopic("light.light2");
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->getId(), "light2");

    auto* devNull = model.findByTopic("light.nonexistent");
    EXPECT_EQ(devNull, nullptr);
}
