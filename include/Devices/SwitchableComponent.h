#pragma once
#include "IDeviceComponent.h"

class HomeDeviceBase;

class SwitchableComponent : public IDeviceComponent
{
public:
    explicit SwitchableComponent(HomeDeviceBase* parent);
    ~SwitchableComponent() override = default;

    std::string name() const override { return "switchable"; }
    void updateState(const QJsonObject& attributes, const QString& stateStr) override;
    std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) override;
    QVariant getProperty(const std::string& key) const override;
    void prepareForCommand(const QString& payload) override;

    bool isOn() const { return m_isOn; }

private:
    HomeDeviceBase* m_parent;
    bool m_isOn = false;
};
