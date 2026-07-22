#include "StoppableComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"

StoppableComponent::StoppableComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void StoppableComponent::updateState(const QJsonObject&, const QString& stateStr)
{
    m_isMoving = (stateStr == "opening" || stateStr == "closing");
}

std::unique_ptr<ICommand> StoppableComponent::parseCommand(const QString& payload, IHaController& controller)
{
    std::string topic = m_parent->getQStringTopic().toStdString();
    auto dotPos = topic.find('.');
    std::string domain = (dotPos != std::string::npos) ? topic.substr(0, dotPos) : "";

    if (payload == "STOP")
    {
        return std::make_unique<GenericHaCommand>(controller, domain, "stop_cover", topic);
    }
    return nullptr;
}

QVariant StoppableComponent::getProperty(const std::string& key) const
{
    if (key == "isMoving")
    {
        return m_isMoving;
    }
    return QVariant();
}

void StoppableComponent::prepareForCommand(const QString& payload)
{
    if (payload == "STOP")
    {
        m_isMoving = false;
    }
    else
    {
        m_isMoving = true;
    }
}
