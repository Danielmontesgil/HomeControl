#pragma once
#include <QObject>
#include <QTimer>
#include <memory>
#include <vector>
#include "IMqttController.h"
#include "ISensor.h"
#include "LightDevice.h"
#include "RollerDevice.h"

class SensorBridge : public QObject
{
    Q_OBJECT
    
    // Q_PROPERTY(float luminosity READ getLuminosity NOTIFY dataChanged)
    // Q_PROPERTY(float temperature READ getTemperature NOTIFY dataChanged)
    
    Q_PROPERTY(float lightStatus READ getLightStatus NOTIFY dataChanged)
    Q_PROPERTY(float rollerPosition READ getRollerPosition NOTIFY dataChanged)
    
public:
    explicit SensorBridge(IMqttController& mqttController, LightDevice& light, RollerDevice& roller, QObject* parent = nullptr);
    virtual ~SensorBridge()=default;
    
    // float getLuminosity() const;
    // float getTemperature() const;
    
    float getLightStatus() const;
    float getRollerPosition() const;
    
    Q_INVOKABLE void toggleLight(bool toggle);
    
signals:
    void dataChanged();
    
private:
    // const std::vector<std::unique_ptr<ISensor>>& m_sensors;
    QTimer *m_timer;
    IMqttController& m_mqttController;
    
    LightDevice& m_light;
    RollerDevice& m_roller;
};
