#include "SensorBridge.h"
#include "DeviceModel.h"
#include "IHaController.h"
#include "HomeDeviceBase.h"
#include "IDeviceFactory.h"
#include "IStoppable.h"
#include <iostream>

SensorBridge::SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel, IHaController& haController, QObject* parent) 
    : QObject(parent), m_haController(haController), m_deviceFactory(deviceFactory), m_deviceModel(deviceModel)
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
            std::string service = (p == "ON") ? "turn_on" : "turn_off";
            m_haController.callService("light", service, entityId);
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

void SensorBridge::onDeviceDiscovered(const QString& type, const QString& entityId, const QString& friendlyName, const QString& state)
{
    if (auto* device = m_deviceModel.findByTopic(entityId))
    {
        device->updateState(state.toStdString(), {});
    }
    else
    {
        addDevice(type, friendlyName, entityId);
        if (auto* newDevice = m_deviceModel.findByTopic(entityId))
        {
            newDevice->updateState(state.toStdString(), {});
        }
    }
}

void SensorBridge::onDeviceStateChanged(const QString& entityId, const QString& state, const QJsonObject& attributes)
{
    if (auto* device = m_deviceModel.findByTopic(entityId))
    {
        device->updateState(state.toStdString(), attributes);
    }
}

