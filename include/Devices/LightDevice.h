#pragma once
#include "HomeDeviceBase.h"
#include "ISwitchable.h"

class LightDevice : public HomeDeviceBase, public ISwitchable
{
public:
    using HomeDeviceBase::HomeDeviceBase;
    
    void onMessageReceived(const std::string& topic, const std::string& payload) override;
    
    DeviceType getType() const override { return DeviceType::Light; };
    
    void prepareForCommand(const std::string& payload) override;
    
    bool isOn() const override { return m_isOn; }

private:
    bool m_isOn {false};
};
