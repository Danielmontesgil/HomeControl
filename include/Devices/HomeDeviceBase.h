#pragma once
#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QJsonObject>
#include <string>

enum class DeviceType
{
    Light,
    Roller
};

class HomeDeviceBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ getQStringId CONSTANT)
    Q_PROPERTY(QString topic READ getQStringTopic CONSTANT)

public:
    explicit HomeDeviceBase(const std::string& id, const std::string& topic, QObject* parent = nullptr) 
        : QObject(parent), id(id), topic(topic) {}
    virtual ~HomeDeviceBase() = default;
    
    std::string getId() const { return id; }
    QString getQStringId() const { return QString::fromStdString(id); }
    QString getQStringTopic() const { return QString::fromStdString(topic); }
    
    virtual DeviceType getType() const = 0; 
    virtual void prepareForCommand(const std::string& payload) = 0;
    
    /**
     * @brief Updates the device state based on data received from Home Assistant.
     * @param state Main reported state (e.g., "on", "off", "open", "closed").
     * @param attributes Extended attributes of the device.
     */
    virtual void updateState(const std::string& state, const QJsonObject& attributes) = 0;
    
signals:
    void updated();
    
protected:
    std::string id;
    std::string topic; // For HA, this stores the entity_id (e.g., "light.living_room_1")
    
    void notifyUpdate()
    {
        QMetaObject::invokeMethod(this, [this]
        {
            emit updated();
        }, Qt::QueuedConnection);
    }
};

