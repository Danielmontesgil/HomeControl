#pragma once
#include <utility>
#include <QString>

class IDeviceFactory;
class ISettingsManager;
class QGuiApplication;
class QTranslator;

namespace Bootstrap::AppSetup {
    void registerDeviceTypes(IDeviceFactory& factory);
    bool setupTranslations(QGuiApplication& app, ISettingsManager& settings, QTranslator& translator);
    std::pair<QString, QString> loadHaCredentials(ISettingsManager& settings);
}
