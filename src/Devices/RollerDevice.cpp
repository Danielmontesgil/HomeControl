#include "RollerDevice.h"
#include "Commands/GenericHaCommand.h"
#include <string>
#include <algorithm>
#include <iostream>

void RollerDevice::updateState(const std::string& state, const QJsonObject& attributes)
{
    // Extract the position (0-100) from the Home Assistant attributes
    if (attributes.contains("current_position"))
    {
        double posPercent = attributes["current_position"].toDouble();
        m_value.set(static_cast<float>(posPercent / 100.0f));
    }

    // Determine if the device is currently moving.
    // Valid states of a cover in HA are: "open", "closed", "opening", "closing"
    bool moving = (state == "opening" || state == "closing");
    if (m_isMoving != moving)
    {
        m_isMoving = moving;
    }

    notifyUpdate();
}

void RollerDevice::stop()
{
    if (m_isMoving) {
        m_isMoving = false;
        notifyUpdate();
    }
}

void RollerDevice::startMoving()
{
    if (!m_isMoving) {
        m_isMoving = true;
        notifyUpdate();
    }
}

void RollerDevice::prepareForCommand(const std::string& payload)
{
    if (payload != "STOP") {
        startMoving();
    }
}

std::unique_ptr<ICommand> RollerDevice::parseCommand(const std::string& payload, IHaController& haController)
{
    if (payload == "STOP") {
        return std::make_unique<GenericHaCommand>(haController, "cover", "stop_cover", topic);
    } else {
        try {
            int pos = std::stoi(payload);
            QJsonObject serviceData;
            serviceData["position"] = pos;
            return std::make_unique<GenericHaCommand>(haController, "cover", "set_cover_position", topic, serviceData);
        } catch (...) {
            std::cerr << "[RollerDevice] Error parsing cover position: " << payload << std::endl;
        }
    }
    return nullptr;
}

