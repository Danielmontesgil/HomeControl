#pragma once
#include <string>
#include <QJsonObject>
#include <QVariant>
#include <QString>
#include <memory>

class ICommand;
class IHaController;

class IDeviceComponent
{
public:
    virtual ~IDeviceComponent() = default;
    virtual std::string name() const = 0;
    virtual void updateState(const QJsonObject& attributes, const QString& stateStr) = 0;
    virtual std::unique_ptr<ICommand> parseCommand(const QString& payload, IHaController& controller) = 0;
    virtual QVariant getProperty(const std::string& key) const = 0;
    virtual void prepareForCommand(const QString& payload) {}
};
