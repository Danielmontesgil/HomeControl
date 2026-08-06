#include "SwitchableComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"

SwitchableComponent::SwitchableComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void SwitchableComponent::setPowerEntityId(const QString& entityId)
{
    m_powerEntityId = entityId.toLower();
}

void SwitchableComponent::updateState(const QJsonObject& attributes, const QString& stateStr)
{
    if (!m_powerEntityId.isEmpty())
    {
        if (attributes.contains("entity_id"))
        {
            QString entityId = attributes["entity_id"].toString().toLower();
            if (entityId == m_powerEntityId)
            {
                m_isOn = (stateStr.toLower() == "on" || stateStr.toLower() == "true");
            }
        }
        return;
    }
    m_isOn = (stateStr.toLower() == "on" || stateStr.toLower() == "cleaning" || stateStr.toLower() == "true");
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
