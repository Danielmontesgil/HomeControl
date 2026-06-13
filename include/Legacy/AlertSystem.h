#pragma once
#include <memory>
#include <vector>
#include "ISensor.h"

class AlertSystem
{
public:
    AlertSystem() = default;
    virtual ~AlertSystem() = default;
    void checkSensors(const std::vector<std::unique_ptr<ISensor>>& sensors);
};
