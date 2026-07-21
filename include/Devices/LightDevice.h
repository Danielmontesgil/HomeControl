#pragma once
#include "HomeDeviceBase.h"
#include "ISwitchable.h"

class LightDevice : public HomeDeviceBase, public ISwitchable
{
public:
    using HomeDeviceBase::HomeDeviceBase;
    
    void updateState(const std::string& state, const QJsonObject& attributes) override;
    
    DeviceType getType() const override { return DeviceType::Light; };
    
    void prepareForCommand(const std::string& payload) override;
    std::unique_ptr<ICommand> parseCommand(const std::string& payload, IHaController& haController) override;
    
    bool isOn() const override { return m_isOn; }

private:
    bool m_isOn {false};
};
