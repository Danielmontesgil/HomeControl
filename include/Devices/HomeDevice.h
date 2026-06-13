#pragma once
#include "IMqttListener.h"
#include "ThreadSafeValue.h"

class HomeDevice : public IMqttListener
{
public:
    explicit HomeDevice(const std::string &id) : id(id), value(0.0f){}
    virtual ~HomeDevice() = default;
    
    float getValue() const { return value.get(); }
    std::string getId() const { return id; }
    
protected:
    std::string id;
    mutable ThreadSafeValue<float> value;
};
