#pragma once
#include "HomeDeviceBase.h"
#include "IStoppable.h"
#include "IValuable.h"
#include "ThreadSafeValue.h"

class RollerDevice : public HomeDeviceBase, public IStoppable, public IValuable
{
public:
    using HomeDeviceBase::HomeDeviceBase;
    
    void updateState(const std::string& state, const QJsonObject& attributes) override;
    
    DeviceType getType() const override { return DeviceType::Roller; };
    
    void prepareForCommand(const std::string& payload) override;
    
    // IStoppable
    bool isMoving() const override { return m_isMoving; }
    void stop() override;
    void startMoving();
    
    // IValuable
    float getValue() const override { return m_value.get(); }

private:
    ThreadSafeValue<float> m_value {0.0f};
    bool m_isMoving {false};
};
