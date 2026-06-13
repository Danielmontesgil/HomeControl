#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor() : sensorName("TemperatureSensor"), sensorValue(0)
{
}

float TemperatureSensor::read() const
{
    float current = sensorValue.get();
    sensorValue.set(current + 2);
    return sensorValue.get();
}

std::string TemperatureSensor::name() const
{
    return sensorName;
}