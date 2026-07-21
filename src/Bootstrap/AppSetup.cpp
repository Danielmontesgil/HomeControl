#include "Bootstrap/AppSetup.h"
#include "IDeviceFactory.h"
#include "ISettingsManager.h"
#include "LightDevice.h"
#include "DimmableColorLightDevice.h"
#include "RollerDevice.h"
#include "VacuumDevice.h"
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
        return std::make_unique<LightDevice>(id, topic);
    });
    factory.registerType("DimmableColorLight", [](const std::string& id, const std::string& topic) {
        return std::make_unique<DimmableColorLightDevice>(id, topic);
    });
    factory.registerType("Roller", [](const std::string& id, const std::string& topic) {
        return std::make_unique<RollerDevice>(id, topic);
    });
    factory.registerType("Vacuum", [](const std::string& id, const std::string& topic) {
        return std::make_unique<VacuumDevice>(id, topic);
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
