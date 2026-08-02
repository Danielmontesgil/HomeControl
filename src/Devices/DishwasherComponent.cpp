#include "DishwasherComponent.h"
#include "HomeDeviceBase.h"
#include "Commands/GenericHaCommand.h"

DishwasherComponent::DishwasherComponent(HomeDeviceBase* parent)
    : m_parent(parent)
{
}

void DishwasherComponent::updateState(const QJsonObject& attributes, const QString& stateStr)
{
    if (!attributes.contains("entity_id")) {
        return;
    }

    QString entityId = attributes["entity_id"].toString().toLower();
    QString stateLower = stateStr.toLower();

    if (entityId.contains("_door"))
    {
        m_doorOpen = (stateLower == "on" || stateLower == "open" || stateLower == "abierto" || stateLower == "true");
    }
    else if (entityId.contains("_operation_state") || entityId.contains("_estado_de_operacion"))
    {
        m_operationState = stateStr;
    }
    else if (entityId.contains("remaining") || entityId.contains("_tiempo_restante"))
    {
        // El tiempo restante suele venir en segundos. Lo convertimos a minutos.
        int seconds = stateStr.toInt();
        m_remainingTime = seconds / 60;
    }
    else if (entityId.contains("_selected_program") || entityId.contains("_programa_seleccionado"))
    {
        m_selectedProgram = stateStr;
    }
}

std::unique_ptr<ICommand> DishwasherComponent::parseCommand(const QString& payload, IHaController& controller)
{
    // Mapear comandos lógicos unificados a las entidades reales del lavavajillas
    if (payload == "ON")
    {
        return std::make_unique<GenericHaCommand>(controller, "switch", "turn_on", "switch.bosch_lavavajillas_power_status");
    }
    else if (payload == "OFF")
    {
        return std::make_unique<GenericHaCommand>(controller, "switch", "turn_off", "switch.bosch_lavavajillas_power_status");
    }
    else if (payload == "START")
    {
        return std::make_unique<GenericHaCommand>(controller, "button", "press", "button.bosch_lavavajillas_start");
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
        return m_remainingTime;
    }
    else if (key == "dishwasherProgram")
    {
        return m_selectedProgram;
    }
    return QVariant();
}
