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

    bool isDoorOpen() const { return m_doorOpen; }
    QString getOperationState() const { return m_operationState; }
    int getRemainingTime() const { return m_remainingTime; }
    QString getSelectedProgram() const { return m_selectedProgram; }

private:
    HomeDeviceBase* m_parent;
    bool m_doorOpen = false;
    QString m_operationState = QStringLiteral("Off");
    int m_remainingTime = 0;
    QString m_selectedProgram = QStringLiteral("None");
};
