#pragma once
#include <QObject>
#include <QTimer>

class DeviceModel;
class IDeviceFactory;
class IMqttController;
class HomeDeviceBase;

class SensorBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(DeviceModel* devices READ getDevices CONSTANT)
    
public:
    explicit SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel, IMqttController& mqttController, QObject* parent = nullptr);
    virtual ~SensorBridge()=default;
    
    Q_INVOKABLE void publishCommand(const QString& topic, const QString& payload);
    Q_INVOKABLE int getDeviceCount(const QString& prefix) const;
    Q_INVOKABLE void addDevice(const QString& type, const QString& id, const QString& topic);
    Q_INVOKABLE int getCountByType(int type) const;
    Q_INVOKABLE void setAllDevicesState(int type, const QString& payload);
    Q_INVOKABLE void stopDevice(const QString& topic);
    
    DeviceModel* getDevices() const { return &m_deviceModel; }
    
signals:
    void countChanged();
    
private:
    IMqttController& m_mqttController;
    IDeviceFactory& m_deviceFactory;
    DeviceModel& m_deviceModel;
};
