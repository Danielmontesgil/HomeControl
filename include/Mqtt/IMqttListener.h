#pragma once
#include <string>

// Interfaz para cualquier objeto que quiera reaccionar a mensajes MQTT
class IMqttListener
{
public:
    virtual void onMessageReceived(const std::string& topic, const std::string& payload) = 0;
    virtual ~IMqttListener() = default;
};
