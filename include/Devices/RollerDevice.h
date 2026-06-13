#pragma once
#include "HomeDevice.h"

class RollerDevice : public HomeDevice
{
public:
    using HomeDevice::HomeDevice;
    
    void onMessageReceived(const std::string& topic, const std::string& payload) override;
};
