#pragma once
#include <string>
#include "ISensor.h"
#include "ThreadSafeValue.h"

class LuminositySensor : public ISensor
{
public:
    LuminositySensor();
    ~LuminositySensor() override;
    
    float read() const override;
    std::string name() const override;
    
private:
    mutable ThreadSafeValue<float> sensorValue = 0;
    std::string sensorName = "";
};
