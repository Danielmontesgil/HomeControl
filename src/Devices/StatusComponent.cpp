#include "StatusComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"
#include <iostream>

StatusComponent::StatusComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void StatusComponent::updateState(const QJsonObject& attributes, const QString& stateStr)
{
    m_state = stateStr;

    if (attributes.contains("battery_level"))
    {
        m_batteryLevel = attributes["battery_level"].toInt();
    }

    if (attributes.contains("fan_speed"))
    {
        m_fanSpeed = attributes["fan_speed"].toString();
    }
}

std::unique_ptr<ICommand> StatusComponent::parseCommand(const QString& payload, IHaController& controller)
{
    std::string topic = m_parent->getQStringTopic().toStdString();
    auto dotPos = topic.find('.');
    std::string domain = (dotPos != std::string::npos) ? topic.substr(0, dotPos) : "";

    if (payload == "LOCATE")
    {
        return std::make_unique<GenericHaCommand>(controller, domain, "locate", topic);
    }
    else if (payload.startsWith("FAN_SPEED:"))
    {
        QString speed = payload.mid(10);
        QJsonObject serviceData;
        serviceData["fan_speed"] = speed;
        return std::make_unique<GenericHaCommand>(controller, domain, "set_fan_speed", topic, serviceData);
    }
    else if (payload.startsWith("SEND_COMMAND:"))
    {
        QString cmd = payload.mid(13);
        QJsonObject serviceData;
        serviceData["command"] = cmd;
        return std::make_unique<GenericHaCommand>(controller, domain, "send_command", topic, serviceData);
    }
    return nullptr;
}

QVariant StatusComponent::getProperty(const std::string& key) const
{
    if (key == "statusState")
    {
        return m_state;
    }
    else if (key == "batteryLevel")
    {
        return m_batteryLevel;
    }
    else if (key == "fanSpeed")
    {
        return m_fanSpeed;
    }
    return QVariant();
}

void StatusComponent::prepareForCommand(const QString& payload)
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
}
