#include <gtest/gtest.h>
#include "Devices/JsonSettingsManager.h"
#include <QFile>
#include <QThreadPool>

class JsonSettingsManagerTest : public ::testing::Test {
protected:
    const std::string testFilename = "test_device_settings.json";

    void SetUp() override {
        // Aseguramos que no existe el archivo antes del test
        QFile::remove(QString::fromStdString(testFilename));
    }

    void TearDown() override {
        // Limpiamos las tareas pendientes del thread pool y eliminamos el archivo después del test
        QThreadPool::globalInstance()->waitForDone();
        QFile::remove(QString::fromStdString(testFilename));
    }
};

TEST_F(JsonSettingsManagerTest, SaveAndGetAlias) {
    JsonSettingsManager manager(testFilename);
    
    // Si no existe, debe retornar el default
    EXPECT_EQ(manager.getAlias("light.living", "Default Name"), "Default Name");
    
    // Guardamos y verificamos en caliente
    manager.saveAlias("light.living", "Luz Salon");
    EXPECT_EQ(manager.getAlias("light.living", "Default Name"), "Luz Salon");
}

TEST_F(JsonSettingsManagerTest, SaveAndGetVisibility) {
    JsonSettingsManager manager(testFilename);
    
    // Default visibility
    EXPECT_TRUE(manager.getVisibility("light.living", true));
    EXPECT_FALSE(manager.getVisibility("light.living", false));
    
    // Guardamos y verificamos
    manager.saveVisibility("light.living", false);
    EXPECT_FALSE(manager.getVisibility("light.living", true));
}

TEST_F(JsonSettingsManagerTest, PersistentReload) {
    {
        JsonSettingsManager manager1(testFilename);
        manager1.saveAlias("light.living", "Luz Persistida");
        manager1.saveVisibility("light.living", false);
        // Garantizamos la finalización de las tareas en el thread pool antes de recargar
        QThreadPool::globalInstance()->waitForDone();
    }
    
    // Creamos una nueva instancia apuntando al mismo archivo físico
    JsonSettingsManager manager2(testFilename);
    EXPECT_EQ(manager2.getAlias("light.living", "Default"), "Luz Persistida");
    EXPECT_FALSE(manager2.getVisibility("light.living", true));
}

TEST_F(JsonSettingsManagerTest, AsyncConcurrentWritesAndPersistence) {
    {
        JsonSettingsManager manager(testFilename);
        
        // Simular múltiples escrituras asíncronas consecutivas y actualizaciones rápido
        manager.saveAlias("light.kitchen", "Cocina");
        manager.saveVisibility("light.kitchen", true);
        manager.saveAlias("sensor.temp", "Temperatura Salon");
        manager.saveVisibility("sensor.temp", false);
        manager.saveAlias("light.kitchen", "Cocina Principal"); // Actualizar alias existente
        
        // Esperamos a que los hilos asíncronos finalicen el volcado a disco
        QThreadPool::globalInstance()->waitForDone();
    }
    
    // Verificar recarga física del archivo guardado de forma asíncrona
    JsonSettingsManager managerReloaded(testFilename);
    EXPECT_EQ(managerReloaded.getAlias("light.kitchen", "Default"), "Cocina Principal");
    EXPECT_TRUE(managerReloaded.getVisibility("light.kitchen", false));
    EXPECT_EQ(managerReloaded.getAlias("sensor.temp", "Default"), "Temperatura Salon");
    EXPECT_FALSE(managerReloaded.getVisibility("sensor.temp", true));
}

