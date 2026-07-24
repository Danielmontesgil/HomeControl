#pragma once
#include "IDeviceComponent.h"
#include <QString>

class HomeDeviceBase;

class ColorableComponent : public IDeviceComponent
{
public:
    explicit ColorableComponent(HomeDeviceBase* parent);
    ~ColorableComponent() override = default;

    std::string name() const override { return "colorable"; }
    void updateState(const QJsonObject& attributes, const QString& stateStr) override;
    std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) override;
    QVariant getProperty(const std::string& key) const override;
    void prepareForCommand(const QString& payload) override;

    QString getColor() const { return m_color; }

private:
    HomeDeviceBase* m_parent;
    QString m_color = "#FFFFFF";
    int m_minColorTemp = 2000;
    int m_maxColorTemp = 6500;
};
