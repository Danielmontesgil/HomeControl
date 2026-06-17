#include "SensorBridge.h"

#include "DeviceModel.h"
#include "IMqttController.h"
#include "HomeDeviceBase.h"
#include "IDeviceFactory.h"

SensorBridge::SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel,IMqttController& mqttController, QObject* parent) 
    : QObject(parent), m_mqttController(mqttController), m_deviceFactory(deviceFactory), m_deviceModel(deviceModel)
{
}

void SensorBridge::publishCommand(const QString& topic, const QString& payload)
{
    m_mqttController.publish(topic.toStdString(), payload.toStdString());
}

int SensorBridge::getDeviceCount(const QString& prefix) const
{
    auto topics = m_mqttController.getRegisteredTopics();
    std::string p = prefix.toStdString();
    int count = 0;
    
    for (const auto& topic : topics)
    {
        if (topic.starts_with(p))
        {
            count++;
        }
    }
    return count;
}

void SensorBridge::addDevice(const QString& type, const QString& id, const QString& topic)
{
    auto device = m_deviceFactory.create(type.toStdString(), id.toStdString(), topic.toStdString());
    m_mqttController.addListener(topic.toStdString(), device.get());
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
            m_mqttController.publish(topic.toStdString(), payload.toStdString());
        }
    }
}
