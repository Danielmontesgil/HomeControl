#include "LightDevice.h"

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
    // No requiere cambios de estado internos previos para una luz ON/OFF simple
}
