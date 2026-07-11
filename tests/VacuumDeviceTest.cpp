#include <gtest/gtest.h>
#include "Devices/VacuumDevice.h"
#include <QJsonObject>

TEST(VacuumDeviceTest, InitialState) {
    VacuumDevice vacuum("Esperancito", "vacuum.esperancito");
    
    EXPECT_EQ(vacuum.getId(), "Esperancito");
    EXPECT_EQ(vacuum.getQStringTopic().toStdString(), "vacuum.esperancito");
    EXPECT_EQ(vacuum.getType(), DeviceType::Vacuum);
    EXPECT_EQ(vacuum.getVacuumState(), "docked");
    EXPECT_EQ(vacuum.getBatteryLevel(), 100);
    EXPECT_EQ(vacuum.getFanSpeed(), "Standard");
}

TEST(VacuumDeviceTest, UpdateStateAndAttributes) {
    VacuumDevice vacuum("Esperancito", "vacuum.esperancito");
    
    QJsonObject attrs;
    attrs["battery_level"] = 85;
    attrs["fan_speed"] = "medium";
    
    vacuum.updateState("cleaning", attrs);
    
    EXPECT_EQ(vacuum.getVacuumState(), "cleaning");
    EXPECT_EQ(vacuum.getBatteryLevel(), 85);
    EXPECT_EQ(vacuum.getFanSpeed(), "medium");
}

TEST(VacuumDeviceTest, UpdateStateWithMissingAttributes) {
    VacuumDevice vacuum("Esperancito", "vacuum.esperancito");
    
    // Si no vienen atributos, el estado se actualiza pero los valores previos se mantienen o usan defaults
    QJsonObject attrs; // Vacío
    vacuum.updateState("docked", attrs);
    
    EXPECT_EQ(vacuum.getVacuumState(), "docked");
    EXPECT_EQ(vacuum.getBatteryLevel(), 100); // Mantiene por defecto
    EXPECT_EQ(vacuum.getFanSpeed(), "Standard");
}

TEST(VacuumDeviceTest, PrepareForCommand) {
    VacuumDevice vacuum("Esperancito", "vacuum.esperancito");
    
    // El método prepareForCommand para VacuumDevice actualmente hereda o procesa localmente
    // si actualiza estados previos al comando de red
    vacuum.prepareForCommand("START");
    EXPECT_EQ(vacuum.getVacuumState(), "cleaning"); // Debe cambiar el estado local inmediatamente
    
    vacuum.prepareForCommand("PAUSE");
    EXPECT_EQ(vacuum.getVacuumState(), "paused");
    
    vacuum.prepareForCommand("RETURN");
    EXPECT_EQ(vacuum.getVacuumState(), "returning");
}
