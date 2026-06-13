#pragma once
#include <string>
#include "IMqttListener.h"

class IMqttController
{
public:
    virtual void connect() = 0;
    virtual void publish(const std::string& topic, const std::string& payload) = 0;
    virtual void subscribe(const std::string& topic) = 0;
    
    // Metodo para añadir observadores (Pattern Observer)
    virtual void addListener(const std::string& topic, IMqttListener* listener) = 0;
    
    virtual ~IMqttController() = default;
};
