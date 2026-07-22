#pragma once
#include "IDeviceComponent.h"

class HomeDeviceBase;

class DimmableComponent : public IDeviceComponent
{
public:
    explicit DimmableComponent(HomeDeviceBase* parent);
    ~DimmableComponent() override = default;

    std::string name() const override { return "dimmable"; }
    void updateState(const QJsonObject& attributes, const QString& stateStr) override;
    std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) override;
    QVariant getProperty(const std::string& key) const override;
    void prepareForCommand(const QString& payload) override;

    float getLevel() const { return m_level; }

private:
    HomeDeviceBase* m_parent;
    float m_level = 0.0f;
};
