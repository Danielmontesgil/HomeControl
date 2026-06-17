#pragma once
#include "HomeDeviceBase.h"

class RollerDevice : public HomeDeviceBase
{
public:
    using HomeDeviceBase::HomeDeviceBase;
    
    void onMessageReceived(const std::string& topic, const std::string& payload) override;
    
    DeviceType getType() const override { return DeviceType::Roller; };
};
