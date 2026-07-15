#include "VacuumDevice.h"
#include "Commands/GenericHaCommand.h"
#include <iostream>

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

std::unique_ptr<ICommand> VacuumDevice::parseCommand(const std::string& payload, IHaController& haController)
{
    if (payload == "START") {
        return std::make_unique<GenericHaCommand>(haController, "vacuum", "start", topic);
    } else if (payload == "PAUSE") {
        return std::make_unique<GenericHaCommand>(haController, "vacuum", "pause", topic);
    } else if (payload == "RETURN" || payload == "DOCK") {
        return std::make_unique<GenericHaCommand>(haController, "vacuum", "return_to_base", topic);
    } else if (payload == "LOCATE") {
        return std::make_unique<GenericHaCommand>(haController, "vacuum", "locate", topic);
    } else if (payload.starts_with("FAN_SPEED:")) {
        std::string speed = payload.substr(10);
        QJsonObject serviceData;
        serviceData["fan_speed"] = QString::fromStdString(speed);
        return std::make_unique<GenericHaCommand>(haController, "vacuum", "set_fan_speed", topic, serviceData);
    } else if (payload.starts_with("SEND_COMMAND:")) {
        std::string cmd = payload.substr(13);
        QJsonObject serviceData;
        serviceData["command"] = QString::fromStdString(cmd);
        return std::make_unique<GenericHaCommand>(haController, "vacuum", "send_command", topic, serviceData);
    }
    return nullptr;
}

