#include "DimmableComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"
#include <iostream>

DimmableComponent::DimmableComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void DimmableComponent::updateState(const QJsonObject& attributes, const QString&)
{
    if (attributes.contains("brightness"))
    {
        m_level = static_cast<float>(attributes["brightness"].toDouble() / 255.0f);
    }
    else if (attributes.contains("current_position"))
    {
        m_level = static_cast<float>(attributes["current_position"].toDouble() / 100.0f);
    }
}

std::unique_ptr<ICommand> DimmableComponent::parseCommand(const QString& payload, IHaController& controller)
{
    std::string topic = m_parent->getQStringTopic().toStdString();
    auto dotPos = topic.find('.');
    std::string domain = (dotPos != std::string::npos) ? topic.substr(0, dotPos) : "";

    if (payload.startsWith("BRIGHTNESS:"))
    {
        try {
            int percent = std::stoi(payload.mid(11).toStdString());
            int haBrightness = static_cast<int>(percent * 255.0f / 100.0f);
            QJsonObject serviceData;
            serviceData["brightness"] = haBrightness;
            return std::make_unique<GenericHaCommand>(controller, domain, "turn_on", topic, serviceData);
        } catch (...) {
            std::cerr << "[DimmableComponent] Error parsing brightness: " << payload.toStdString() << std::endl;
        }
    }
    else if (domain == "cover")
    {
        try {
            int pos = std::stoi(payload.toStdString());
            QJsonObject serviceData;
            serviceData["position"] = pos;
            return std::make_unique<GenericHaCommand>(controller, domain, "set_cover_position", topic, serviceData);
        } catch (...) {
            // Not a numeric position payload, could be STOP
        }
    }
    return nullptr;
}

QVariant DimmableComponent::getProperty(const std::string& key) const
{
    if (key == "level")
    {
        return m_level;
    }
    return QVariant();
}

void DimmableComponent::prepareForCommand(const QString& payload)
{
    if (payload.startsWith("BRIGHTNESS:"))
    {
        try {
            int br = std::stoi(payload.mid(11).toStdString());
            m_level = static_cast<float>(br / 100.0f);
        } catch (...) {}
    }
    else
    {
        try {
            int pos = std::stoi(payload.toStdString());
            m_level = static_cast<float>(pos / 100.0f);
        } catch (...) {}
    }
}
