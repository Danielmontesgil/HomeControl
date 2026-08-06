#include "HomeDeviceBase.h"
#include "Commands/ICommand.h"

HomeDeviceBase::HomeDeviceBase(const std::string& id, const std::string& topic, DeviceType type, QObject* parent)
    : QObject(parent), id(id), topic(topic), m_type(type), m_available(true)
{
}

void HomeDeviceBase::addComponent(std::unique_ptr<IDeviceComponent> component)
{
    if (component)
    {
        std::string name = component->name();
        m_components[name] = std::move(component);
    }
}

IDeviceComponent* HomeDeviceBase::getComponent(const std::string& name) const
{
    auto it = m_components.find(name);
    if (it != m_components.end())
    {
        return it->second.get();
    }
    return nullptr;
}

bool HomeDeviceBase::hasComponent(const std::string& name) const
{
    return m_components.find(name) != m_components.end();
}

QStringList HomeDeviceBase::getComponentNames() const
{
    QStringList names;
    for (const auto& [name, _] : m_components)
    {
        names.append(QString::fromStdString(name));
    }
    return names;
}

void HomeDeviceBase::prepareForCommand(const std::string& payload)
{
    QString qPayload = QString::fromStdString(payload);
    for (auto& [_, component] : m_components)
    {
        component->prepareForCommand(qPayload);
    }
    notifyUpdate();
}

std::unique_ptr<ICommand> HomeDeviceBase::parseCommand(const std::string& payload, IHaController& haController)
{
    QString qPayload = QString::fromStdString(payload);
    for (auto& [_, component] : m_components)
    {
        auto command = component->parseCommand(qPayload, haController);
        if (command)
        {
            return command;
        }
    }
    return nullptr;
}

void HomeDeviceBase::updateState(const std::string& state, const QJsonObject& attributes)
{
    bool nextAvailable = (state != "unavailable" && state != "unknown");

    if (m_type == DeviceType::Dishwasher && attributes.contains(QStringLiteral("entity_id")))
    {
        QString entityId = attributes[QStringLiteral("entity_id")].toString().toLower();
        bool isPowerSwitch = entityId.startsWith(QStringLiteral("switch.")) && 
                             (entityId.contains(QStringLiteral("power_status")) || entityId.contains(QStringLiteral("power")));
        
        if (!isPowerSwitch && !nextAvailable)
        {
            nextAvailable = m_available;
        }
    }

    if (m_available != nextAvailable)
    {
        m_available = nextAvailable;
        emit availableChanged();
    }

    QString qState = QString::fromStdString(state);
    for (auto& [_, component] : m_components)
    {
        component->updateState(attributes, qState);
    }
    notifyUpdate();
}
