#include "DishwasherComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"
#include <QDateTime>

DishwasherComponent::DishwasherComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void DishwasherComponent::setEntityMappings(const QVariantMap& mappings)
{
    m_powerEntityId = mappings.value(QStringLiteral("power")).toString().toLower();
    m_doorEntityId = mappings.value(QStringLiteral("door")).toString().toLower();
    m_operationStateEntityId = mappings.value(QStringLiteral("operation_state")).toString().toLower();
    m_remainingTimeEntityId = mappings.value(QStringLiteral("remaining_time")).toString().toLower();
    m_selectedProgramEntityId = mappings.value(QStringLiteral("selected_program")).toString().toLower();
    m_programsSelectEntityId = mappings.value(QStringLiteral("programs")).toString().toLower();
    m_startEntityId = mappings.value(QStringLiteral("start")).toString().toLower();
    m_stopEntityId = mappings.value(QStringLiteral("stop")).toString().toLower();
    m_extraDryEntityId = mappings.value(QStringLiteral("extradry")).toString().toLower();
    m_halfLoadEntityId = mappings.value(QStringLiteral("half_load")).toString().toLower();
    m_speedPerfectEntityId = mappings.value(QStringLiteral("speedperfect")).toString().toLower();
}

void DishwasherComponent::updateState(const QJsonObject& attributes, const QString& stateStr)
{
    if (!attributes.contains("entity_id")) {
        return;
    }

    QString entityId = attributes["entity_id"].toString().toLower();
    QString stateLower = stateStr.toLower();

    // 1. Door sensor
    if (!m_doorEntityId.isEmpty() && entityId == m_doorEntityId)
    {
        m_doorOpen = (stateLower == "on" || stateLower == "open" || stateLower == "true");
    }
    // 2. Operation State sensor
    else if (!m_operationStateEntityId.isEmpty() && entityId == m_operationStateEntityId)
    {
        m_operationState = stateStr;
        if (stateLower == "off" || stateLower == "inactive")
        {
            resetToDefaults();
        }
    }
    // 3. Remaining Time sensor
    else if (!m_remainingTimeEntityId.isEmpty() && entityId == m_remainingTimeEntityId)
    {
        if (stateStr.contains('T') && stateStr.contains(':'))
        {
            QDateTime targetTime = QDateTime::fromString(stateStr, Qt::ISODate);
            if (targetTime.isValid())
            {
                qint64 secs = QDateTime::currentDateTimeUtc().secsTo(targetTime.toUTC());
                m_remainingTime = static_cast<int>(qMax(0LL, secs / 60));
            }
        }
        else if (stateStr.contains(':'))
        {
            // Formato de duración como "4:40:00" o "04:40"
            QStringList parts = stateStr.split(':');
            if (parts.size() >= 2)
            {
                int hours = parts[0].toInt();
                int minutes = parts[1].toInt();
                m_remainingTime = hours * 60 + minutes;
            }
        }
        else
        {
            int seconds = stateStr.toInt();
            m_remainingTime = seconds / 60;
        }
    }
    // 4. Selected Program sensor
    else if (!m_selectedProgramEntityId.isEmpty() && entityId == m_selectedProgramEntityId)
    {
        m_selectedProgram = stateStr;
    }
    // 5. Power switch entity tracker
    else if (!m_powerEntityId.isEmpty() && entityId == m_powerEntityId)
    {
        if (stateLower == "off" || stateLower == "false" || stateLower == "0")
        {
            resetToDefaults();
        }
    }
    // 6. ExtraDry switch
    else if (!m_extraDryEntityId.isEmpty() && entityId == m_extraDryEntityId)
    {
        m_extraDry = (stateLower == "on" || stateLower == "true");
    }
    // 7. Half Load switch
    else if (!m_halfLoadEntityId.isEmpty() && entityId == m_halfLoadEntityId)
    {
        m_halfLoad = (stateLower == "on" || stateLower == "true");
    }
    // 8. SpeedPerfect switch
    else if (!m_speedPerfectEntityId.isEmpty() && entityId == m_speedPerfectEntityId)
    {
        m_speedPerfect = (stateLower == "on" || stateLower == "true");
    }
}

std::unique_ptr<ICommand> DishwasherComponent::parseCommand(const QString& payload, IHaController& controller)
{
    if (payload == "ON")
    {
        if (m_powerEntityId.isEmpty()) return nullptr;
        return std::make_unique<GenericHaCommand>(controller, "switch", "turn_on", m_powerEntityId.toStdString());
    }
    else if (payload == "OFF")
    {
        if (m_powerEntityId.isEmpty()) return nullptr;
        return std::make_unique<GenericHaCommand>(controller, "switch", "turn_off", m_powerEntityId.toStdString());
    }
    else if (payload == "START")
    {
        if (m_startEntityId.isEmpty()) return nullptr;
        return std::make_unique<GenericHaCommand>(controller, "button", "press", m_startEntityId.toStdString());
    }
    else if (payload == "STOP")
    {
        if (m_stopEntityId.isEmpty()) return nullptr;
        return std::make_unique<GenericHaCommand>(controller, "button", "press", m_stopEntityId.toStdString());
    }
    else if (payload == "TOGGLE_EXTRADRY")
    {
        if (m_extraDryEntityId.isEmpty()) return nullptr;
        std::string service = m_extraDry ? "turn_off" : "turn_on";
        return std::make_unique<GenericHaCommand>(controller, "switch", service, m_extraDryEntityId.toStdString());
    }
    else if (payload == "TOGGLE_HALFLOAD")
    {
        if (m_halfLoadEntityId.isEmpty()) return nullptr;
        std::string service = m_halfLoad ? "turn_off" : "turn_on";
        return std::make_unique<GenericHaCommand>(controller, "switch", service, m_halfLoadEntityId.toStdString());
    }
    else if (payload == "TOGGLE_SPEEDPERFECT")
    {
        if (m_speedPerfectEntityId.isEmpty()) return nullptr;
        std::string service = m_speedPerfect ? "turn_off" : "turn_on";
        return std::make_unique<GenericHaCommand>(controller, "switch", service, m_speedPerfectEntityId.toStdString());
    }
    else if (payload.startsWith(QLatin1String("SELECT_PROGRAM:")))
    {
        QString targetEntity = m_programsSelectEntityId.isEmpty() ? m_selectedProgramEntityId : m_programsSelectEntityId;
        if (targetEntity.isEmpty()) return nullptr;
        QString program = payload.mid(15);
        QJsonObject data;
        data[QStringLiteral("option")] = program;
        return std::make_unique<GenericHaCommand>(controller, "select", "select_option", targetEntity.toStdString(), data);
    }
    return nullptr;
}

QVariant DishwasherComponent::getProperty(const std::string& key) const
{
    if (key == "dishwasherDoorOpen")
    {
        return m_doorOpen;
    }
    else if (key == "dishwasherState")
    {
        return m_operationState;
    }
    else if (key == "dishwasherRemainingTime")
    {
        return getRemainingTime();
    }
    else if (key == "dishwasherProgram")
    {
        return m_selectedProgram;
    }
    else if (key == "dishwasherExtraDry")
    {
        return m_extraDry;
    }
    else if (key == "dishwasherHalfLoad")
    {
        return m_halfLoad;
    }
    else if (key == "dishwasherSpeedPerfect")
    {
        return m_speedPerfect;
    }
    return QVariant();
}

int DishwasherComponent::getRemainingTime() const
{
    QString opState = m_operationState.toLower();
    if (opState.contains(QLatin1String("run")) || opState.contains(QLatin1String("running")))
    {
        return m_remainingTime;
    }
    return 0;
}

void DishwasherComponent::resetToDefaults()
{
    m_extraDry = false;
    m_halfLoad = false;
    m_speedPerfect = false;
    m_selectedProgram = QStringLiteral("Dishcare.Dishwasher.Program.Eco50");
    m_remainingTime = 0;
}
