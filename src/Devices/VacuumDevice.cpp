#include "VacuumDevice.h"

VacuumDevice::VacuumDevice(const std::string& id, const std::string& topic, QObject* parent)
    : HomeDeviceBase(id, topic, parent)
{
}

DeviceType VacuumDevice::getType() const
{
    return DeviceType::Vacuum;
}

void VacuumDevice::prepareForCommand(const std::string& payload)
{
    if (payload == "START")
    {
        m_state = "cleaning";
    }
    else if (payload == "PAUSE")
    {
        m_state = "paused";
    }
    else if (payload == "RETURN" || payload == "DOCK")
    {
        m_state = "returning";
    }
    notifyUpdate();
}

void VacuumDevice::updateState(const std::string& state, const QJsonObject& attributes)
{
    m_state = state;

    if (attributes.contains("battery_level"))
    {
        m_batteryLevel = attributes["battery_level"].toInt();
    }

    if (attributes.contains("fan_speed"))
    {
        m_fanSpeed = attributes["fan_speed"].toString().toStdString();
    }

    notifyUpdate();
}

QString VacuumDevice::getVacuumState() const
{
    return QString::fromStdString(m_state);
}

int VacuumDevice::getBatteryLevel() const
{
    return m_batteryLevel;
}

QString VacuumDevice::getFanSpeed() const
{
    return QString::fromStdString(m_fanSpeed);
}
