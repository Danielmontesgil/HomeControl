#pragma once
#include "HomeDevice.h"

class LightDevice : public HomeDevice
{
public:
    using HomeDevice::HomeDevice;
    
    void onMessageReceived(const std::string& topic, const std::string& payload) override;
};
