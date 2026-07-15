#include "Commands/GenericHaCommand.h"
#include "IHaController.h"

GenericHaCommand::GenericHaCommand(IHaController& controller, const std::string& domain, const std::string& service, const std::string& entityId, const QJsonObject& data)
    : m_controller(controller), m_domain(domain), m_service(service), m_entityId(entityId), m_data(data)
{
}

void GenericHaCommand::execute()
{
    m_controller.callService(m_domain, m_service, m_entityId, m_data);
}
