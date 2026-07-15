#pragma once
#include "ICommand.h"
#include <string>
#include <QJsonObject>

class IHaController;

class GenericHaCommand : public ICommand {
public:
    explicit GenericHaCommand(IHaController& controller, const std::string& domain, const std::string& service, const std::string& entityId, const QJsonObject& data = {});
    ~GenericHaCommand() override = default;

    void execute() override;

private:
    IHaController& m_controller;
    std::string m_domain;
    std::string m_service;
    std::string m_entityId;
    QJsonObject m_data;
};
