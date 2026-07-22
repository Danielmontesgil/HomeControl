#include "SwitchableComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"

SwitchableComponent::SwitchableComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void SwitchableComponent::updateState(const QJsonObject&, const QString& stateStr)
{
    m_isOn = (stateStr == "on" || stateStr == "cleaning" || stateStr == "true");
}

std::unique_ptr<ICommand> SwitchableComponent::parseCommand(const QString& payload, IHaController& controller)
{
    std::string topic = m_parent->getQStringTopic().toStdString();
    auto dotPos = topic.find('.');
    std::string domain = (dotPos != std::string::npos) ? topic.substr(0, dotPos) : "";

    if (payload == "ON" || payload == "START")
    {
        std::string service = (domain == "vacuum") ? "start" : "turn_on";
        return std::make_unique<GenericHaCommand>(controller, domain, service, topic);
    }
    else if (payload == "OFF" || payload == "PAUSE")
    {
        std::string service = (domain == "vacuum") ? "pause" : "turn_off";
        return std::make_unique<GenericHaCommand>(controller, domain, service, topic);
    }
    else if (payload == "RETURN" || payload == "DOCK")
    {
        if (domain == "vacuum")
        {
            return std::make_unique<GenericHaCommand>(controller, domain, "return_to_base", topic);
        }
    }
    return nullptr;
}

QVariant SwitchableComponent::getProperty(const std::string& key) const
{
    if (key == "isOn")
    {
        return m_isOn;
    }
    return QVariant();
}

void SwitchableComponent::prepareForCommand(const QString& payload)
{
    if (payload == "ON" || payload == "START")
    {
        m_isOn = true;
    }
    else if (payload == "OFF" || payload == "PAUSE")
    {
        m_isOn = false;
    }
}
