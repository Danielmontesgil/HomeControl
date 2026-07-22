#pragma once
#include "IDeviceComponent.h"

class HomeDeviceBase;

class StoppableComponent : public IDeviceComponent
{
public:
    explicit StoppableComponent(HomeDeviceBase* parent);
    ~StoppableComponent() override = default;

    std::string name() const override { return "stoppable"; }
    void updateState(const QJsonObject& attributes, const QString& stateStr) override;
    std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) override;
    QVariant getProperty(const std::string& key) const override;
    void prepareForCommand(const QString& payload) override;

    bool isMoving() const { return m_isMoving; }

private:
    HomeDeviceBase* m_parent;
    bool m_isMoving = false;
};
