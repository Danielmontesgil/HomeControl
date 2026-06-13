#include "LuminositySensor.h"

LuminositySensor::LuminositySensor() : sensorName("LuminositySensor"), sensorValue(0)
{
}

float LuminositySensor::read() const
{
	float current = sensorValue.get();
	sensorValue.set(current + 1);
	return sensorValue.get();
}

std::string LuminositySensor::name() const
{
	return sensorName;
}

LuminositySensor::~LuminositySensor() = default;

