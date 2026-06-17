#include "RollerDevice.h"
#include <string>
#include <algorithm> // Para std::remove_if

void RollerDevice::onMessageReceived(const std::string& topic, const std::string& payload)
{
    if (payload.empty()) return;

    try {
        std::string clean = payload;
        
        // Saneamiento robusto: Eliminamos todo lo que no sea número o punto decimal
        // Esto cubre "45.5%", " 45.5", "Valor: 45.5", etc.
        std::erase_if(clean, [](const unsigned char c) {
            return !std::isdigit(c) && c != '.';
        });

        if (!clean.empty()) {
            const float pos = std::stof(clean) / 100.0f;
            updateValue(pos);
        }
    } catch (...) {
        // En un sistema real, aquí registraríamos un log de error
    }
}
