#include "SensorBridge.h"
#include "DeviceModel.h"
#include "IHaController.h"
#include "HomeDeviceBase.h"
#include "Commands/ICommand.h"
#include "IDeviceFactory.h"
#include "ISettingsManager.h"
#include <iostream>
#include <QColor>
#include <QJsonArray>
#include <QGuiApplication>
#include <QClipboard>

SensorBridge::SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel, IHaController& haController, ISettingsManager& settingsManager, QObject* parent) 
    : QObject(parent), m_haController(haController), m_deviceFactory(deviceFactory), m_deviceModel(deviceModel), m_settingsManager(settingsManager)
{
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

