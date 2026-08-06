#pragma once
#include "IDeviceComponent.h"
#include <QString>

class HomeDeviceBase;

class DishwasherComponent : public IDeviceComponent
{
public:
    explicit DishwasherComponent(HomeDeviceBase* parent);
    ~DishwasherComponent() override = default;

    std::string name() const override { return "dishwasher"; }
    void updateState(const QJsonObject& attributes, const QString& stateStr) override;
    std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) override;
    QVariant getProperty(const std::string& key) const override;
    void setEntityMappings(const QVariantMap& mappings);

    bool isDoorOpen() const { return m_doorOpen; }
    QString getOperationState() const { return m_operationState; }
    int getRemainingTime() const;
    QString getSelectedProgram() const { return m_selectedProgram; }
    bool isExtraDry() const { return m_extraDry; }
    bool isHalfLoad() const { return m_halfLoad; }
    bool isSpeedPerfect() const { return m_speedPerfect; }

private:
    HomeDeviceBase* m_parent;
    bool m_doorOpen = false;
    QString m_operationState = QStringLiteral("Off");
    int m_remainingTime = 0;
    QString m_selectedProgram = QStringLiteral("None");

    // Dynamic Entity IDs configured
    QString m_doorEntityId;
    QString m_operationStateEntityId;
    QString m_remainingTimeEntityId;
    QString m_selectedProgramEntityId;
    QString m_programsSelectEntityId;
    QString m_powerEntityId;
    QString m_startEntityId;
    QString m_stopEntityId;
    QString m_extraDryEntityId;
    QString m_halfLoadEntityId;
    QString m_speedPerfectEntityId;

    // Advanced features state
    bool m_extraDry = false;
    bool m_halfLoad = false;
    bool m_speedPerfect = false;

    void resetToDefaults();
};
