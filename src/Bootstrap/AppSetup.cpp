#include "Bootstrap/AppSetup.h"
#include "IDeviceFactory.h"
#include "ISettingsManager.h"
#include "HomeDeviceBase.h"
#include "SwitchableComponent.h"
#include "DimmableComponent.h"
#include "ColorableComponent.h"
#include "StoppableComponent.h"
#include "StatusComponent.h"
#include <QGuiApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

namespace Bootstrap::AppSetup {

void registerDeviceTypes(IDeviceFactory& factory)
{
    factory.registerType("Light", [](const std::string& id, const std::string& topic) {
        auto dev = std::make_unique<HomeDeviceBase>(id, topic, DeviceType::Light);
        dev->addComponent(std::make_unique<SwitchableComponent>(dev.get()));
        return dev;
    });
    factory.registerType("DimmableColorLight", [](const std::string& id, const std::string& topic) {
        auto dev = std::make_unique<HomeDeviceBase>(id, topic, DeviceType::Light);
        dev->addComponent(std::make_unique<SwitchableComponent>(dev.get()));
        dev->addComponent(std::make_unique<DimmableComponent>(dev.get()));
        dev->addComponent(std::make_unique<ColorableComponent>(dev.get()));
        return dev;
    });
    factory.registerType("Roller", [](const std::string& id, const std::string& topic) {
        auto dev = std::make_unique<HomeDeviceBase>(id, topic, DeviceType::Roller);
        dev->addComponent(std::make_unique<DimmableComponent>(dev.get()));
        dev->addComponent(std::make_unique<StoppableComponent>(dev.get()));
        return dev;
    });
    factory.registerType("Vacuum", [](const std::string& id, const std::string& topic) {
        auto dev = std::make_unique<HomeDeviceBase>(id, topic, DeviceType::Vacuum);
        dev->addComponent(std::make_unique<SwitchableComponent>(dev.get()));
        dev->addComponent(std::make_unique<StatusComponent>(dev.get()));
        return dev;
    });
}

bool setupTranslations(QGuiApplication& app, ISettingsManager& settings, QTranslator& translator)
{
    std::string savedLang = settings.getAlias("system.language", "system");
    QLocale localeToLoad = (savedLang == "system") ? QLocale::system() : QLocale(QString::fromStdString(savedLang));

    if (localeToLoad.language() != QLocale::English)
    {
        if (translator.load(localeToLoad, "HomeControl", "_", ":/i18n"))
        {
            app.installTranslator(&translator);
            return true;
        }
        else
        {
            std::cerr << "[i18n] Fallo al cargar el archivo de traduccion para: " 
                      << localeToLoad.name().toStdString() << std::endl;
        }
    }
    return false;
}

std::pair<QString, QString> loadHaCredentials(ISettingsManager& settings)
{
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

    return {haUrl, haToken};
}

}
