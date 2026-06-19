#include "LightDevice.h"

void LightDevice::onMessageReceived(const std::string&, const std::string& payload)
{
    bool newState = (payload == "ON");
    if (m_isOn != newState) {
        m_isOn = newState;
        notifyUpdate();
    }
}

void LightDevice::prepareForCommand(const std::string& payload)
{
    // No requiere cambios de estado internos previos para una luz ON/OFF simple
}
