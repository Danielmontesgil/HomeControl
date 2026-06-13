#pragma once
#include <string>

class ISensor
{
public:
    virtual float read() const = 0;
    virtual std::string name() const = 0;
    
    virtual ~ISensor() = default;
};