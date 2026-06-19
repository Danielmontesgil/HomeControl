#include "RollerDevice.h"
#include <string>
#include <algorithm>

void RollerDevice::onMessageReceived(const std::string&, const std::string& payload)
{
    if (payload.empty()) return;

    try {
        std::string clean = payload;
        std::erase_if(clean, [](const unsigned char c) {
            return !std::isdigit(c) && c != '.';
        });

        if (!clean.empty()) {
            const float pos = std::stof(clean) / 100.0f;
            m_value.set(pos);
            
            // Si llega un reporte de valor, asumimos que se detuvo o está en el target
            // (En un sistema real compararíamos con el target)
            m_isMoving = false; 
            notifyUpdate();
        }
    } catch (...) {}
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
