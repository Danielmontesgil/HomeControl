#pragma once
#include "HomeDeviceBase.h"

class VacuumDevice : public HomeDeviceBase
{
    Q_OBJECT
    Q_PROPERTY(QString vacuumState READ getVacuumState NOTIFY updated)
    Q_PROPERTY(int batteryLevel READ getBatteryLevel NOTIFY updated)
    Q_PROPERTY(QString fanSpeed READ getFanSpeed NOTIFY updated)

public:
    explicit VacuumDevice(const std::string& id, const std::string& topic, QObject* parent = nullptr);
    ~VacuumDevice() override = default;

    DeviceType getType() const override;
    void prepareForCommand(const std::string& payload) override;
    void updateState(const std::string& state, const QJsonObject& attributes) override;

    QString getVacuumState() const;
    int getBatteryLevel() const;
    QString getFanSpeed() const;

private:
    std::string m_state{"docked"};
    int m_batteryLevel{100};
    std::string m_fanSpeed{"Standard"};
};
