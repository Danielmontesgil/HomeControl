#include "DimmableColorLightDevice.h"
#include <QJsonArray>
#include <QColor>

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
