#include "LightDevice.h"
#include "Commands/GenericHaCommand.h"

void LightDevice::updateState(const std::string& state, const QJsonObject&)
{
    bool newState = (state == "on");
    if (m_isOn != newState) {
        m_isOn = newState;
        notifyUpdate();
    }
}

void LightDevice::prepareForCommand(const std::string& payload)
{
}

std::unique_ptr<ICommand> LightDevice::parseCommand(const std::string& payload, IHaController& haController)
{
    if (payload == "ON") {
        return std::make_unique<GenericHaCommand>(haController, "light", "turn_on", topic);
    } else if (payload == "OFF") {
        return std::make_unique<GenericHaCommand>(haController, "light", "turn_off", topic);
    }
    return nullptr;
}
