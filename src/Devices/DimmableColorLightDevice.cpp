#include "DimmableColorLightDevice.h"
#include "Commands/GenericHaCommand.h"
#include <QJsonArray>
#include <QColor>
#include <iostream>

void DimmableColorLightDevice::updateState(const std::string& state, const QJsonObject& attributes)
{
    // Update switchable state
    bool onState = (state == "on");
    bool changed = (m_isOn != onState);
    m_isOn = onState;

    // Update valuable state (brightness)
    // Home Assistant reports brightness in the range 0 - 255
    if (attributes.contains("brightness"))
    {
        float brVal = static_cast<float>(attributes["brightness"].toDouble() / 255.0f);
        if (m_brightness != brVal)
        {
            m_brightness = brVal;
            changed = true;
        }
    }

    // Update colorable state (rgb_color)
    // Home Assistant reports this as a JSON array of 3 integers: [R, G, B]
    if (attributes.contains("rgb_color"))
    {
        QJsonArray rgbArray = attributes["rgb_color"].toArray();
        if (rgbArray.size() >= 3)
        {
            int r = rgbArray[0].toInt();
            int g = rgbArray[1].toInt();
            int b = rgbArray[2].toInt();
            QString hexColor = QString("#%1%2%3")
                .arg(r, 2, 16, QChar('0'))
                .arg(g, 2, 16, QChar('0'))
                .arg(b, 2, 16, QChar('0')).toUpper();

            if (m_color != hexColor)
            {
                m_color = hexColor;
                changed = true;
            }
        }
    }

    if (changed)
    {
        notifyUpdate();
    }
}

void DimmableColorLightDevice::prepareForCommand(const std::string& payload)
{
    // Apply immediate local state feedback for smoother animations/transitions
    if (payload == "ON")
    {
        m_isOn = true;
        notifyUpdate();
    }
    else if (payload == "OFF")
    {
        m_isOn = false;
        notifyUpdate();
    }
    else if (payload.starts_with("BRIGHTNESS:"))
    {
        try {
            int br = std::stoi(payload.substr(11));
            m_brightness = static_cast<float>(br / 100.0f);
            m_isOn = (br > 0);
            notifyUpdate();
        } catch (...) {}
    }
    else if (payload.starts_with("COLOR:"))
    {
        m_color = QString::fromStdString(payload.substr(6));
        m_isOn = true;
        notifyUpdate();
    }
}

std::unique_ptr<ICommand> DimmableColorLightDevice::parseCommand(const std::string& payload, IHaController& haController)
{
    if (payload == "ON")
    {
        return std::make_unique<GenericHaCommand>(haController, "light", "turn_on", topic);
    }
    else if (payload == "OFF")
    {
        return std::make_unique<GenericHaCommand>(haController, "light", "turn_off", topic);
    }
    else if (payload.starts_with("BRIGHTNESS:"))
    {
        try {
            int percent = std::stoi(payload.substr(11));
            int haBrightness = static_cast<int>(percent * 255.0f / 100.0f);
            QJsonObject serviceData;
            serviceData["brightness"] = haBrightness;
            return std::make_unique<GenericHaCommand>(haController, "light", "turn_on", topic, serviceData);
        } catch (...) {
            std::cerr << "[DimmableColorLightDevice] Error parsing brightness command: " << payload << std::endl;
        }
    }
    else if (payload.starts_with("COLOR:"))
    {
        std::string hexStr = payload.substr(6);
        QColor color(QString::fromStdString(hexStr));
        if (color.isValid()) {
            QJsonArray rgbArray;
            rgbArray.append(color.red());
            rgbArray.append(color.green());
            rgbArray.append(color.blue());
            QJsonObject serviceData;
            serviceData["rgb_color"] = rgbArray;
            return std::make_unique<GenericHaCommand>(haController, "light", "turn_on", topic, serviceData);
        } else {
            std::cerr << "[DimmableColorLightDevice] Invalid color format: " << hexStr << std::endl;
        }
    }
    return nullptr;
}

