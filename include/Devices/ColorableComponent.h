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
    bool supportsColorTemp() const { return m_supportsColorTemp; }
    int getColorTemp() const { return m_colorTemp; }
    int getMinColorTemp() const { return m_minColorTemp; }
    int getMaxColorTemp() const { return m_maxColorTemp; }

private:
    HomeDeviceBase* m_parent;
    QString m_color = "#FFFFFF";
    int m_minColorTemp = 2000;
    int m_maxColorTemp = 6500;
    int m_colorTemp = 4000;
    bool m_supportsColorTemp = false;
};
