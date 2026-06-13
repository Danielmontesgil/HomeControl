#pragma once
#include "ISensor.h"
#include "ThreadSafeValue.h"

class TemperatureSensor : public ISensor
{
public:
    TemperatureSensor();
    ~TemperatureSensor() override = default;
    
    float read() const override;
    std::string name() const override;
    
private:
    mutable ThreadSafeValue<float> sensorValue = 0;
    std::string sensorName = "";
};
