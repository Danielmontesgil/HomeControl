#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QTranslator>
#include <iostream>
#include <memory>
#include "DeviceFactory.h"
#include "HaWebSocketController.h"
#include "SensorBridge.h"
#include "DeviceModel.h"
#include "HaImageProvider.h"
#include "JsonSettingsManager.h"
#include "Bootstrap/AppSetup.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    QQuickStyle::setStyle("Fusion");
    
    auto haController = HaWebSocketController();
    auto deviceFactory = DeviceFactory();
    auto deviceModel = DeviceModel();
    
    Bootstrap::AppSetup::registerDeviceTypes(deviceFactory);
    
    qmlRegisterType<DeviceModel>("SensorsApp", 1, 0, "DeviceModel");
    
    JsonSettingsManager settings("device_settings.json");
    
    SensorBridge bridge(deviceFactory, deviceModel, haController, settings);
    
    QTranslator translator;
    Bootstrap::AppSetup::setupTranslations(app, settings, translator);
    
    QObject::connect(&haController, &HaWebSocketController::deviceDiscovered, &bridge, &SensorBridge::onDeviceDiscovered);
    QObject::connect(&haController, &HaWebSocketController::deviceStateChanged, &bridge, &SensorBridge::onDeviceStateChanged);

    auto [haUrl, haToken] = Bootstrap::AppSetup::loadHaCredentials(settings);

    bridge.setHaCredentials(haUrl, haToken);
    haController.connectToHa(haUrl.toStdString(), haToken.toStdString());

    QQmlApplicationEngine engine;
    engine.addImageProvider("hacamera", new HaImageProvider(haUrl, haToken));
    
    engine.rootContext()->setContextProperty("sensorBridge", &bridge);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &w : warnings) {
            std::cerr << "QML Error: " << w.toString().toStdString() << std::endl;
        }
    });
    
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
