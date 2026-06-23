#pragma once
#include <string>
#include <QJsonObject>

/**
 * @brief Interface for the Home Assistant controller.
 * Defines basic network operations decoupled from the concrete implementation.
 */
class IHaController
{
public:
    virtual ~IHaController() = default;

    /**
     * @brief Connects to the Home Assistant server.
     * @param url WebSocket URL (e.g., "ws://localhost:8123/api/websocket").
     * @param token Long-Lived Access Token.
     */
    virtual void connectToHa(const std::string& url, const std::string& token) = 0;

    /**
     * @brief Sends a service call to Home Assistant.
     * @param domain Service domain (e.g., "light", "cover").
     * @param service Service name (e.g., "turn_on", "set_cover_position").
     * @param entityId Entity identifier (e.g., "light.living_room").
     * @param serviceData Additional parameters in JSON format.
     */
    virtual void callService(const std::string& domain, 
                             const std::string& service, 
                             const std::string& entityId, 
                             const QJsonObject& serviceData = {}) = 0;
};
