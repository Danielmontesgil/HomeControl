#include "LightDevice.h"
#include <iostream>
#include <ostream>

void LightDevice::onMessageReceived(const std::string& topic, const std::string& payload)
{
    if (payload == "ON")
    {
        value.set(1.0f);
    }
    else if (payload == "OFF")
    {
        value.set(0.0f);
    }
    
    std::cout << "LightDevice::onMessageReceived()" << std::endl;
}
