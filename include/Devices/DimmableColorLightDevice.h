#pragma once
#include "HomeDeviceBase.h"
#include "ISwitchable.h"
#include "IValuable.h"
#include "IColorable.h"

/**
 * @brief Represents a smart light bulb that supports turning on/off, dimming, and color control.
 */
class DimmableColorLightDevice : public HomeDeviceBase, public ISwitchable, public IValuable, public IColorable
{
public:
    using HomeDeviceBase::HomeDeviceBase;

    // HomeDeviceBase overrides
    DeviceType getType() const override { return DeviceType::Light; }
    void prepareForCommand(const std::string& payload) override;
    std::unique_ptr<ICommand> parseCommand(const std::string& payload, IHaController& haController) override;
    void updateState(const std::string& state, const QJsonObject& attributes) override;

    // ISwitchable overrides
    bool isOn() const override { return m_isOn; }

    // IValuable overrides (Brightness 0.0f - 1.0f)
    float getValue() const override { return m_brightness; }

    // IColorable overrides
    QString getColor() const override { return m_color; }
    void setColor(const QString& hexColor) override { m_color = hexColor; }

private:
    bool m_isOn{false};
    float m_brightness{0.0f};
    QString m_color{"#FFFFFF"};
};
