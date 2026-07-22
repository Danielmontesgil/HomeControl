#pragma once
#include "IDeviceComponent.h"
#include <QString>

class HomeDeviceBase;

class StatusComponent : public IDeviceComponent
{
public:
    explicit StatusComponent(HomeDeviceBase* parent);
    ~StatusComponent() override = default;

    std::string name() const override { return "status"; }
    void updateState(const QJsonObject& attributes, const QString& stateStr) override;
    std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) override;
    QVariant getProperty(const std::string& key) const override;
    void prepareForCommand(const QString& payload) override;

    QString getStatusState() const { return m_state; }
    int getBatteryLevel() const { return m_batteryLevel; }
    QString getFanSpeed() const { return m_fanSpeed; }

private:
    HomeDeviceBase* m_parent;
    QString m_state;
    int m_batteryLevel = 0;
    QString m_fanSpeed;
};
