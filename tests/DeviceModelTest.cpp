#include <gtest/gtest.h>
#include "Devices/DeviceModel.h"
#include "Devices/HomeDeviceBase.h"
#include "Devices/SwitchableComponent.h"
#include "Devices/DimmableComponent.h"
#include "Devices/ColorableComponent.h"
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

    auto dev = std::make_unique<HomeDeviceBase>("light1", "light.light1", DeviceType::Light);
    model.addDevice(std::move(dev));
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_TRUE(aboutToBeInsertedEmitted);
    EXPECT_TRUE(insertedEmitted);
}

TEST(DeviceModelTest, RoleMapping) {
    DeviceModel model;
    auto dev = std::make_unique<HomeDeviceBase>("light1", "light.light1", DeviceType::Light);
    dev->addComponent(std::make_unique<SwitchableComponent>(dev.get()));
    dev->addComponent(std::make_unique<DimmableComponent>(dev.get()));
    dev->addComponent(std::make_unique<ColorableComponent>(dev.get()));
    model.addDevice(std::move(dev));

    QModelIndex index = model.index(0);

    EXPECT_EQ(model.data(index, DeviceModel::IdRole).toString(), "light1");
    EXPECT_EQ(model.data(index, DeviceModel::TopicRole).toString(), "light.light1");
    EXPECT_FLOAT_EQ(model.data(index, DeviceModel::ValueRole).toFloat(), 0.0f);
    EXPECT_FALSE(model.data(index, DeviceModel::IsOnRole).toBool());
    EXPECT_TRUE(model.data(index, DeviceModel::SupportsColorRole).toBool());
    EXPECT_EQ(model.data(index, DeviceModel::ColorRole).toString(), "#FFFFFF");
    EXPECT_EQ(model.data(index, DeviceModel::TypeRole).toInt(), static_cast<int>(DeviceType::Light));
    
    QStringList caps = model.data(index, DeviceModel::CapabilitiesRole).toStringList();
    EXPECT_TRUE(caps.contains("switchable"));
    EXPECT_TRUE(caps.contains("dimmable"));
    EXPECT_TRUE(caps.contains("colorable"));
}

TEST(DeviceModelTest, DataChangedSignal) {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    DeviceModel model;
    auto dev = std::make_unique<HomeDeviceBase>("light1", "light.light1", DeviceType::Light);
    dev->addComponent(std::make_unique<DimmableComponent>(dev.get()));
    auto* devPtr = dev.get();
    model.addDevice(std::move(dev));

    bool dataChangedEmitted = false;
    QObject::connect(&model, &DeviceModel::dataChanged, [&](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
        dataChangedEmitted = true;
        EXPECT_EQ(topLeft.row(), 0);
        EXPECT_EQ(bottomRight.row(), 0);
    });

    devPtr->prepareForCommand("BRIGHTNESS:85");
    QCoreApplication::processEvents();

    EXPECT_TRUE(dataChangedEmitted);
}

TEST(DeviceModelTest, FindByTopic) {
    DeviceModel model;
    model.addDevice(std::make_unique<HomeDeviceBase>("light1", "light.light1", DeviceType::Light));
    model.addDevice(std::make_unique<HomeDeviceBase>("light2", "light.light2", DeviceType::Light));

    auto* dev = model.findByTopic("light.light2");
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->getId(), "light2");

    auto* devNull = model.findByTopic("light.nonexistent");
    EXPECT_EQ(devNull, nullptr);
}
