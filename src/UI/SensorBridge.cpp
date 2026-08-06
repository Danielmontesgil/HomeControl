#include "SensorBridge.h"
#include "Core/Log.h"
#include "DeviceModel.h"
#include "IHaController.h"
#include "HomeDeviceBase.h"
#include "Commands/ICommand.h"
#include "IDeviceFactory.h"
#include "ISettingsManager.h"
#include "Devices/DishwasherComponent.h"
#include "Devices/SwitchableComponent.h"
#include <QColor>
#include <QJsonArray>
#include <QGuiApplication>
#include <QClipboard>

SensorBridge::SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel, IHaController& haController, ISettingsManager& settingsManager, ILicenseManager& licenseManager, QObject* parent) 
    : QObject(parent), m_haController(haController), m_deviceFactory(deviceFactory), m_deviceModel(deviceModel), m_settingsManager(settingsManager), m_licenseManager(licenseManager), m_internetAccessModel(this)
{
    m_mappingManager.loadMappings(QStringLiteral("config.json"));
}

void SensorBridge::publishCommand(const QString& topic, const QString& payload)
{
    if (auto* device = m_deviceModel.findByTopic(topic)) {
        std::string cmd = payload.toStdString();
        device->prepareForCommand(cmd);
        if (auto command = device->parseCommand(cmd, m_haController)) {
            command->execute();
        }
    }
}


int SensorBridge::getDeviceCount(const QString& prefix) const
{
    int count = 0;
    for (int i = 0; i < m_deviceModel.rowCount(); ++i)
    {
        auto idx = m_deviceModel.index(i);
        QString topic = m_deviceModel.data(idx, DeviceModel::TopicRole).toString();
        if (topic.startsWith(prefix))
        {
            count++;
        }
    }
    return count;
}

void SensorBridge::addDevice(const QString& type, const QString& id, const QString& topic)
{
    // Avoid duplication if the device is already registered
    if (m_deviceModel.findByTopic(topic)) return;

    auto device = m_deviceFactory.create(type.toStdString(), id.toStdString(), topic.toStdString());
    m_deviceModel.addDevice(std::move(device));
    emit countChanged();
}

int SensorBridge::getCountByType(int type) const
{
    int count = 0;
    for (int i = 0; i < m_deviceModel.rowCount(); ++i)
    {
        auto idx = m_deviceModel.index(i);
        if (m_deviceModel.data(idx, DeviceModel::TypeRole).toInt() == type)
        {
            count++;
        }
    }
    return count;
}

void SensorBridge::setAllDevicesState(int type, const QString& payload)
{
    for (int i = 0; i < m_deviceModel.rowCount(); ++i)
    {
        auto idx = m_deviceModel.index(i);
        if (m_deviceModel.data(idx, DeviceModel::TypeRole).toInt() == type)
        {
            QString topic = m_deviceModel.data(idx, DeviceModel::TopicRole).toString();
            publishCommand(topic, payload);
        }
    }
}

void SensorBridge::stopDevice(const QString& topic)
{
    if (auto* device = m_deviceModel.findByTopic(topic))
    {
        device->prepareForCommand("STOP");
        m_haController.callService("cover", "stop_cover", topic.toStdString());
    }
}

void SensorBridge::onDeviceDiscovered(const QString& type, const QString& entityId, const QString& friendlyName, const QString& state, const QJsonObject& attributes)
{
    // 1. Intercept mapped compound devices
    if (m_mappingManager.isEntityMapped(entityId))
    {
        QString targetTopic = m_mappingManager.getDeviceTopicForEntity(entityId);
        QString targetName = m_mappingManager.getDeviceNameForEntity(entityId);
        QString targetType = m_mappingManager.getDeviceTypeForEntity(entityId);

        auto* device = m_deviceModel.findByTopic(targetTopic);
        if (!device)
        {
            addDevice(targetType, targetName, targetTopic);
            device = m_deviceModel.findByTopic(targetTopic);

            if (device && targetType == QStringLiteral("Dishwasher"))
            {
                auto mappings = m_mappingManager.getEntityMappingsForDevice(targetTopic);
                if (auto* dw = device->getComponent("dishwasher"))
                {
                    auto* dwComp = static_cast<DishwasherComponent*>(dw);
                    dwComp->setEntityMappings(mappings);
                }
                if (auto* sw = device->getComponent("switchable"))
                {
                    auto* swComp = static_cast<SwitchableComponent*>(sw);
                    swComp->setPowerEntityId(mappings.value(QStringLiteral("power")).toString());
                }
            }
        }

        if (device)
        {
            QJsonObject modifiedAttributes = attributes;
            modifiedAttributes[QStringLiteral("entity_id")] = entityId;
            device->updateState(state.toStdString(), modifiedAttributes);
        }
        return;
    }

    bool isExcluded = entityId.startsWith(QStringLiteral("light.")) || 
                      entityId.startsWith(QStringLiteral("cover.")) || 
                      entityId.startsWith(QStringLiteral("vacuum."));

    if (entityId.startsWith("switch.") || type == "Switch")
    {
        if (entityId.contains("_internet_access_") || entityId.contains("internet") || entityId.contains("acceso_a_internet"))
        {
            if (isParentalPremium())
            {
                m_internetAccessModel.handleSwitchUpdate(entityId, state, friendlyName);
            }
        }
        return;
    }

    if (entityId.startsWith("device_tracker.") || type == "DeviceTracker")
    {
        if (isParentalPremium())
        {
            m_internetAccessModel.handleTrackerUpdate(entityId, state, friendlyName);
        }
        return;
    }

    if (type == QStringLiteral("Sensor") || type == QStringLiteral("BinarySensor") || type == QStringLiteral("Button"))
    {
        return;
    }

    if (auto* device = m_deviceModel.findByTopic(entityId))
    {
        device->updateState(state.toStdString(), attributes);
    }
    else
    {
        std::string finalName = m_settingsManager.getAlias(entityId.toStdString(), friendlyName.toStdString());
        addDevice(type, QString::fromStdString(finalName), entityId);
        if (auto* newDevice = m_deviceModel.findByTopic(entityId))
        {
            newDevice->updateState(state.toStdString(), attributes);
        }
    }
}

void SensorBridge::renameDevice(const QString& topic, const QString& newName)
{
    if (auto* device = m_deviceModel.findByTopic(topic))
    {
        device->setId(newName.toStdString());
        m_settingsManager.saveAlias(topic.toStdString(), newName.toStdString());
    }
}

void SensorBridge::onDeviceStateChanged(const QString& entityId, const QString& state, const QJsonObject& attributes)
{
    // 1. Intercept mapped compound devices
    if (m_mappingManager.isEntityMapped(entityId))
    {
        QString targetTopic = m_mappingManager.getDeviceTopicForEntity(entityId);
        if (auto* device = m_deviceModel.findByTopic(targetTopic))
        {
            QJsonObject modifiedAttributes = attributes;
            modifiedAttributes[QStringLiteral("entity_id")] = entityId;
            device->updateState(state.toStdString(), modifiedAttributes);
        }
        return;
    }

    if (entityId.startsWith("switch.") && (entityId.contains("_internet_access_") || entityId.contains("internet") || entityId.contains("acceso_a_internet")))
    {
        if (isParentalPremium())
        {
            QString friendlyName = attributes.contains("friendly_name") ? attributes["friendly_name"].toString() : entityId;
            m_internetAccessModel.handleSwitchUpdate(entityId, state, friendlyName);
        }
        return;
    }

    if (entityId.startsWith("device_tracker."))
    {
        if (isParentalPremium())
        {
            QString friendlyName = attributes.contains("friendly_name") ? attributes["friendly_name"].toString() : entityId;
            m_internetAccessModel.handleTrackerUpdate(entityId, state, friendlyName);
        }
        return;
    }

    if (auto* device = m_deviceModel.findByTopic(entityId))
    {
        device->updateState(state.toStdString(), attributes);
    }
}

void SensorBridge::setHaCredentials(const QString& url, const QString& token)
{
    m_haUrl = url;
    m_haToken = token;
}

QString SensorBridge::getHaMapUrl(const QString& entityId) const
{
    QString httpUrl = m_haUrl;
    httpUrl.replace("ws://", "http://");
    httpUrl.replace("wss://", "https://");
    httpUrl.replace("/api/websocket", "/api/camera_proxy/" + entityId);
    
    if (!m_haToken.isEmpty())
    {
        httpUrl += "?token=" + m_haToken;
    }
    return httpUrl;
}

void SensorBridge::saveHaCredentials(const QString& url, const QString& token)
{
    m_settingsManager.saveAlias("system.ha_url", url.toStdString());
    m_settingsManager.saveAlias("system.ha_token", token.toStdString());
    setHaCredentials(url, token);
    m_haController.connectToHa(url.toStdString(), token.toStdString());
}

QString SensorBridge::getSavedHaUrl() const
{
    std::string url = m_settingsManager.getAlias("system.ha_url", m_haUrl.toStdString());
    return QString::fromStdString(url);
}

QString SensorBridge::getSavedHaToken() const
{
    std::string token = m_settingsManager.getAlias("system.ha_token", m_haToken.toStdString());
    return QString::fromStdString(token);
}

QString SensorBridge::getSavedLanguage() const
{
    std::string lang = m_settingsManager.getAlias("system.language", "system");
    return QString::fromStdString(lang);
}

void SensorBridge::saveLanguage(const QString& lang)
{
    m_settingsManager.saveAlias("system.language", lang.toStdString());
}

bool SensorBridge::isHaConnected() const
{
    return m_haController.isConnected();
}

int SensorBridge::getHaLatency() const
{
    return m_haController.getLatencyMs();
}

int SensorBridge::getHaReconnectAttempts() const
{
    return m_haController.getReconnectAttempts();
}

int SensorBridge::getHaNextReconnectDelay() const
{
    return m_haController.getNextReconnectDelayMs();
}

QString SensorBridge::getHaLastDisconnectReason() const
{
    return QString::fromStdString(m_haController.getLastDisconnectReason());
}

bool SensorBridge::isDebugBuild() const
{
#ifdef QT_DEBUG
    return true;
#else
    return false;
#endif
}

bool SensorBridge::isVerboseLoggingEnabled() const
{
    return m_haController.isVerboseLoggingEnabled();
}

void SensorBridge::setVerboseLogging(bool enable)
{
    m_haController.setVerboseLogging(enable);
}

void SensorBridge::forceDisconnect()
{
    m_haController.forceDisconnect();
}

void SensorBridge::setSimulationLatency(int ms)
{
    m_haController.setSimulationLatency(ms);
}

void SensorBridge::setSimulationAuthFail(bool enable)
{
    m_haController.setSimulationAuthFail(enable);
}

void SensorBridge::setSimulationOfflineMode(bool enable)
{
    m_haController.setSimulationOfflineMode(enable);
}

void SensorBridge::reconnect()
{
    m_haController.connectToHa(getSavedHaUrl().toStdString(), getSavedHaToken().toStdString());
}

void SensorBridge::copyToClipboard(const QString& text)
{
    if (auto* clipboard = QGuiApplication::clipboard())
    {
        clipboard->setText(text);
    }
}

unsigned int SensorBridge::getLogMask() const
{
    return Log::getLogMask();
}

void SensorBridge::setLogMask(unsigned int mask)
{
    if (Log::getLogMask() != mask)
    {
        Log::setLogMask(mask);
        emit logMaskChanged();
    }
}

void SensorBridge::forceDeviceUpdate(const QString& entityId)
{
    m_haController.callService("homeassistant", "update_entity", entityId.toStdString());
}

void SensorBridge::toggleInternet(const QString& entityId, bool enabled)
{
    QString service = enabled ? QStringLiteral("turn_on") : QStringLiteral("turn_off");
    m_haController.callService("switch", service.toStdString(), entityId.toStdString());
}

bool SensorBridge::isParentalPremium() const
{
    return m_licenseManager.isFeatureLicensed("parental");
}

bool SensorBridge::activateParentalControl(const QString& licenseKey)
{
    bool success = m_licenseManager.activateFeature("parental", licenseKey.toStdString());
    if (success)
    {
        emit licenseChanged();
        reconnect();
    }
    return success;
}

void SensorBridge::deactivateParentalControl()
{
    m_licenseManager.activateFeature("parental", "");
    m_internetAccessModel.clear();
    emit licenseChanged();
    reconnect();
}

void SensorBridge::saveSetting(const QString& key, const QString& value)
{
    m_settingsManager.saveAlias(key.toStdString(), value.toStdString());
}

QString SensorBridge::getSetting(const QString& key, const QString& defaultValue) const
{
    return QString::fromStdString(m_settingsManager.getAlias(key.toStdString(), defaultValue.toStdString()));
}

