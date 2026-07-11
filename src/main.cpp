#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include "DeviceFactory.h"
#include "LightDevice.h"
#include "DimmableColorLightDevice.h"
#include "LuminositySensor.h"
#include "HaWebSocketController.h"
#include "RollerDevice.h"
#include "TemperatureSensor.h"
#include "SensorBridge.h"
#include "DeviceModel.h"
#include "VacuumDevice.h"
#include "HaImageProvider.h"
#include "JsonSettingsManager.h"

using namespace Qt::StringLiterals;

void updateSensors(std::vector<std::unique_ptr<ISensor>>& sensors, std::atomic<bool>& running)
{
    while (running)
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
    
    QQuickStyle::setStyle("Fusion");
    
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<LuminositySensor>());
    sensors.push_back(std::make_unique<TemperatureSensor>());
    
    std::atomic<bool> keepRunning(true);
    std::thread worker(updateSensors, std::ref(sensors), std::ref(keepRunning));
    
    // --- Network Setup (Home Assistant WebSockets) ---
    auto haController = HaWebSocketController();

    // --- Device Factory & Model Setup ---
    auto deviceFactory = DeviceFactory();
    auto deviceModel = DeviceModel();
    
    // Register types in factory (Open/Closed Principle)
    deviceFactory.registerType("Light", [](const std::string& id, const std::string& topic) {
        return std::make_unique<LightDevice>(id, topic);
    });
    deviceFactory.registerType("DimmableColorLight", [](const std::string& id, const std::string& topic) {
        return std::make_unique<DimmableColorLightDevice>(id, topic);
    });
    deviceFactory.registerType("Roller", [](const std::string& id, const std::string& topic) {
        return std::make_unique<RollerDevice>(id, topic);
    });
    deviceFactory.registerType("Vacuum", [](const std::string& id, const std::string& topic) {
        return std::make_unique<VacuumDevice>(id, topic);
    });
    
    // Register type for QML model roles exposure
    qmlRegisterType<DeviceModel>("SensorsApp", 1, 0, "DeviceModel");
    
    // Initialize Settings Manager
    JsonSettingsManager settings("device_settings.json");
    
    // Initialize SensorBridge (injecting dependencies)
    SensorBridge bridge(deviceFactory, deviceModel, haController, settings);
    
    // Connect HA controller signals to Bridge slots
    // This must be done BEFORE connecting to prevent missing the initial state dump (get_states)
    QObject::connect(&haController, &HaWebSocketController::deviceDiscovered, &bridge, &SensorBridge::onDeviceDiscovered);
    QObject::connect(&haController, &HaWebSocketController::deviceStateChanged, &bridge, &SensorBridge::onDeviceStateChanged);

    // Load HA credentials dynamically: first check persistent settings, then fallback to local config.json file
    std::string savedUrl = settings.getAlias("system.ha_url", "");
    std::string savedToken = settings.getAlias("system.ha_token", "");
    
    QString haUrl = QString::fromStdString(savedUrl);
    QString haToken = QString::fromStdString(savedToken);

    if (haUrl.isEmpty() || haToken.isEmpty())
    {
        QFile configFile("config.json");
        if (configFile.open(QIODevice::ReadOnly))
        {
            QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
            QJsonObject obj = doc.object();
            if (obj.contains("ha_url")) haUrl = obj["ha_url"].toString();
            if (obj.contains("ha_token")) haToken = obj["ha_token"].toString();
        }
        else
        {
            std::cerr << "[Warning] config.json and saved settings not found, using default URL and empty token." << std::endl;
            haUrl = "ws://localhost:8123/api/websocket";
        }
    }

    if (haUrl.startsWith("ws://") && !haUrl.contains("localhost") && !haUrl.contains("127.0.0.1"))
    {
        std::cerr << "[Security Warning] Usando WebSocket no cifrado (ws://) fuera de localhost para transmitir credenciales de Home Assistant." << std::endl;
    }

    bridge.setHaCredentials(haUrl, haToken);
    haController.connectToHa(haUrl.toStdString(), haToken.toStdString());

    QQmlApplicationEngine engine;
    engine.addImageProvider("hacamera", new HaImageProvider(haUrl, haToken));
    
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
        keepRunning = false;
        if (worker.joinable())
        {
            worker.join();
        }
        return -1;
    }
    
    int result = app.exec();
    keepRunning = false;
    if (worker.joinable())
    {
        worker.join();
    }
    return result;
}
