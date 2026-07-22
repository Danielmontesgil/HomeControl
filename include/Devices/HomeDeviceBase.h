#pragma once
#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <string>
#include <memory>
#include <unordered_map>
#include "IDeviceComponent.h"

class ICommand;
class IHaController;

enum class DeviceType
{
    Light,
    Roller,
    Vacuum
};

class HomeDeviceBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ getQStringId CONSTANT)
    Q_PROPERTY(QString topic READ getQStringTopic CONSTANT)

public:
    explicit HomeDeviceBase(const std::string& id, const std::string& topic, DeviceType type, QObject* parent = nullptr);
    virtual ~HomeDeviceBase() = default;

    std::string getId() const { return id; }
    void setId(const std::string& newId) { id = newId; notifyUpdate(); }
    QString getQStringId() const { return QString::fromStdString(id); }
    QString getQStringTopic() const { return QString::fromStdString(topic); }

    DeviceType getType() const { return m_type; }

    void addComponent(std::unique_ptr<IDeviceComponent> component);
    IDeviceComponent* getComponent(const std::string& name) const;
    bool hasComponent(const std::string& name) const;
    QStringList getComponentNames() const;

    void prepareForCommand(const std::string& payload);
    std::unique_ptr<ICommand> parseCommand(const std::string& payload, IHaController& haController);
    void updateState(const std::string& state, const QJsonObject& attributes);

signals:
    void updated();

protected:
    std::string id;
    std::string topic; // For HA, this stores the entity_id (e.g., "light.living_room_1")
    DeviceType m_type;
    std::unordered_map<std::string, std::unique_ptr<IDeviceComponent>> m_components;

    void notifyUpdate()
    {
        QMetaObject::invokeMethod(this, [this]
        {
            emit updated();
        }, Qt::QueuedConnection);
    }
};
