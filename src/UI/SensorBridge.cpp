#include "SensorBridge.h"

SensorBridge::SensorBridge(IMqttController& mqttController, LightDevice& light, RollerDevice& roller, QObject* parent) : QObject(parent), m_mqttController(mqttController), m_light(light), m_roller(roller)
{
    m_timer = new QTimer(this);
    
    connect(m_timer, &QTimer::timeout, this, &SensorBridge::dataChanged);
    
    m_timer->start(100);
}

// float SensorBridge::getLuminosity() const
// {
//     if (m_sensors.size() > 0)
//     {
//         return m_sensors[0]->read();
//     }
//     return 0.0f;
// }
//
// float SensorBridge::getTemperature() const
// {
//     if (m_sensors.size() > 1)
//     {
//         return m_sensors[1]->read();
//     }
//     return 0.0f;
// }

float SensorBridge::getLightStatus() const
{
    return m_light.getValue();
}

float SensorBridge::getRollerPosition() const
{
    return m_roller.getValue();
}

void SensorBridge::toggleLight(bool toggle)
{
    m_mqttController.publish("home/light/living", toggle ? "ON" : "OFF");
}
