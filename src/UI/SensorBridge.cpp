#include "SensorBridge.h"
#include "DeviceModel.h"
#include "IHaController.h"
#include "HomeDeviceBase.h"
#include "IDeviceFactory.h"
#include "IStoppable.h"
#include "ISettingsManager.h"
#include <iostream>
#include <QColor>
#include <QJsonArray>

SensorBridge::SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel, IHaController& haController, ISettingsManager& settingsManager, QObject* parent) 
    : QObject(parent), m_haController(haController), m_deviceFactory(deviceFactory), m_deviceModel(deviceModel), m_settingsManager(settingsManager)
{
}

void SensorBridge::publishCommand(const QString& topic, const QString& payload)
{
    std::string entityId = topic.toStdString();
    std::string p = payload.toStdString();
    
    // Get the corresponding device in the local model
    if (auto* device = m_deviceModel.findByTopic(topic)) {
        // Execute immediate local changes if UI needs it before network (e.g. roller animation)
        device->prepareForCommand(p);
        
        // Map QML command to the corresponding HA service call
        if (device->getType() == DeviceType::Light) {
            if (p == "ON") {
                m_haController.callService("light", "turn_on", entityId);
            } else if (p == "OFF") {
                m_haController.callService("light", "turn_off", entityId);
            } else if (p.starts_with("BRIGHTNESS:")) {
                try {
                    int percent = std::stoi(p.substr(11));
                    int haBrightness = static_cast<int>(percent * 255.0f / 100.0f);
                    QJsonObject serviceData;
                    serviceData["brightness"] = haBrightness;
                    m_haController.callService("light", "turn_on", entityId, serviceData);
                } catch (...) {
                    std::cerr << "[SensorBridge] Error parsing brightness command: " << p << std::endl;
                }
            } else if (p.starts_with("COLOR:")) {
                std::string hexStr = p.substr(6);
                QColor color(QString::fromStdString(hexStr));
                if (color.isValid()) {
                    QJsonArray rgbArray;
                    rgbArray.append(color.red());
                    rgbArray.append(color.green());
                    rgbArray.append(color.blue());
                    QJsonObject serviceData;
                    serviceData["rgb_color"] = rgbArray;
                    m_haController.callService("light", "turn_on", entityId, serviceData);
                } else {
                    std::cerr << "[SensorBridge] Invalid color format: " << hexStr << std::endl;
                }
            }
        }
        else if (device->getType() == DeviceType::Roller) {
            if (p == "STOP") {
                m_haController.callService("cover", "stop_cover", entityId);
            } else {
                try {
                    int pos = std::stoi(p);
                    QJsonObject serviceData;
                    serviceData["position"] = pos;
                    m_haController.callService("cover", "set_cover_position", entityId, serviceData);
                } catch (...) {
                    std::cerr << "[SensorBridge] Error parsing cover position: " << p << std::endl;
                }
            }
        }
        else if (device->getType() == DeviceType::Vacuum) {
            if (p == "START") {
                m_haController.callService("vacuum", "start", entityId);
            } else if (p == "PAUSE") {
                m_haController.callService("vacuum", "pause", entityId);
            } else if (p == "RETURN" || p == "DOCK") {
                m_haController.callService("vacuum", "return_to_base", entityId);
            } else if (p == "LOCATE") {
                m_haController.callService("vacuum", "locate", entityId);
            } else if (p.starts_with("FAN_SPEED:")) {
                std::string speed = p.substr(10);
                QJsonObject serviceData;
                serviceData["fan_speed"] = QString::fromStdString(speed);
                m_haController.callService("vacuum", "set_fan_speed", entityId, serviceData);
            } else if (p.starts_with("SEND_COMMAND:")) {
                std::string cmd = p.substr(13);
                QJsonObject serviceData;
                serviceData["command"] = QString::fromStdString(cmd);
                m_haController.callService("vacuum", "send_command", entityId, serviceData);
            }
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
        if (auto* stoppable = dynamic_cast<IStoppable*>(device))
        {
            stoppable->stop();
        }
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

