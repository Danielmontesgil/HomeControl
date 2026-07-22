#include "ColorableComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"
#include <QJsonArray>
#include <QColor>
#include <iostream>

ColorableComponent::ColorableComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void ColorableComponent::updateState(const QJsonObject& attributes, const QString&)
{
    if (attributes.contains("rgb_color"))
    {
        QJsonArray rgbArray = attributes["rgb_color"].toArray();
        if (rgbArray.size() >= 3)
        {
            int r = rgbArray[0].toInt();
            int g = rgbArray[1].toInt();
            int b = rgbArray[2].toInt();
            m_color = QString("#%1%2%3")
                .arg(r, 2, 16, QChar('0'))
                .arg(g, 2, 16, QChar('0'))
                .arg(b, 2, 16, QChar('0')).toUpper();
        }
    }
}

std::unique_ptr<ICommand> ColorableComponent::parseCommand(const QString& payload, IHaController& controller)
{
    std::string topic = m_parent->getQStringTopic().toStdString();
    auto dotPos = topic.find('.');
    std::string domain = (dotPos != std::string::npos) ? topic.substr(0, dotPos) : "";

    if (payload.startsWith("COLOR:"))
    {
        QString hexStr = payload.mid(6);
        QColor color(hexStr);
        if (color.isValid())
        {
            QJsonArray rgbArray;
            rgbArray.append(color.red());
            rgbArray.append(color.green());
            rgbArray.append(color.blue());
            QJsonObject serviceData;
            serviceData["rgb_color"] = rgbArray;
            return std::make_unique<GenericHaCommand>(controller, domain, "turn_on", topic, serviceData);
        }
        else
        {
            std::cerr << "[ColorableComponent] Invalid color format: " << hexStr.toStdString() << std::endl;
        }
    }
    return nullptr;
}

QVariant ColorableComponent::getProperty(const std::string& key) const
{
    if (key == "color")
    {
        return m_color;
    }
    return QVariant();
}

void ColorableComponent::prepareForCommand(const QString& payload)
{
    if (payload.startsWith("COLOR:"))
    {
        m_color = payload.mid(6);
    }
}
