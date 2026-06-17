#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include "ConsoleLogger.h"
#include "DeviceFactory.h"
#include "LightDevice.h"
#include "LuminositySensor.h"
#include "MqttController.h"
#include "RollerDevice.h"
#include "TemperatureSensor.h"
#include "SensorBridge.h"
#include "DeviceModel.h"

using namespace Qt::StringLiterals;

void updateSensors(std::vector<std::unique_ptr<ISensor>>& sensors)
{
    while (true)
    {
        for (auto& sensor : sensors)
        {
            sensor->read();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<LuminositySensor>());
    sensors.push_back(std::make_unique<TemperatureSensor>());
    
    std::thread worker(updateSensors, std::ref(sensors));
    worker.detach();
    
    auto mqttController = MqttController("tcp://localhost:1883", "HomeControl_Daniel");
    mqttController.connect();

    // --- Configuración de la Factoría de Dispositivos ---
    auto deviceFactory = DeviceFactory();
    
    auto deviceModel = DeviceModel();
    
    // Registro de tipos (Open/Closed Principle)
    deviceFactory.registerType("Light", [](const std::string& id, const std::string& topic) {
        return std::make_unique<LightDevice>(id, topic);
    });
    deviceFactory.registerType("Roller", [](const std::string& id, const std::string& topic) {
        return std::make_unique<RollerDevice>(id, topic);
    });
    
    ConsoleLogger logger;
    mqttController.addListener("*", &logger);
    
    mqttController.subscribe("home/#");
    
    // Registro de tipos para que QML reconozca el modelo y sus roles
    qmlRegisterType<DeviceModel>("SensorsApp", 1, 0, "DeviceModel");
    
    // Ahora inyectamos todos los dispositivos en el bridge de forma dinámica
    SensorBridge bridge( deviceFactory, deviceModel, mqttController);
    bridge.addDevice("Light", "LivingRoomLight1", "home/light/living/1");
    bridge.addDevice("Light", "LivingRoomLight2", "home/light/living/2");
    bridge.addDevice("Light", "KitchenLight1", "home/light/kitchen/1");
    bridge.addDevice("Roller", "LivingRoomRoller1", "home/roller/living/1");

    QQmlApplicationEngine engine;
    
    engine.rootContext()->setContextProperty("sensorBridge", &bridge);

    // Capturamos errores de QML para verlos en la consola de Rider
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &w : warnings) {
            std::cerr << "QML Error: " << w.toString().toStdString() << std::endl;
        }
    });
    
    // Al usar RESOURCE_PREFIX "/", la ruta es mas directa
    const QUrl url(u"qrc:/SensorsApp/main.qml"_s);
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl)
        {
            if (!obj && url == objUrl)
            {
                std::cerr << "CRITICAL: Fallo al crear el objeto QML." << std::endl;
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);
    
    engine.load(url);
    
    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }
    
    return app.exec();
}
