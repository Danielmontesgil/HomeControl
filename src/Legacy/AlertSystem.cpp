#include "AlertSystem.h"
#include "ISensor.h"
#include <iostream>

void AlertSystem::checkSensors(const std::vector<std::unique_ptr<ISensor>>& sensors)
{
    for (auto& sensor : sensors)
    {
        if (sensor->read() >= 50)
        {
            std::cout << "¡ALERTA! Valor crítico en " << sensor->name() << std::endl;
        }
    }
}

