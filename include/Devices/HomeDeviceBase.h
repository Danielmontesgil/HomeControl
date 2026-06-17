#pragma once
#include <QObject>
#include <QMetaObject>
#include <QString>
#include "IMqttListener.h"
#include "ThreadSafeValue.h"

enum class DeviceType
{
    Light,
    Roller
};

class HomeDeviceBase : public QObject, public IMqttListener
{
    Q_OBJECT
    Q_PROPERTY(QString id READ getQStringId CONSTANT)
    Q_PROPERTY(QString topic READ getQStringTopic CONSTANT)
    Q_PROPERTY(float value READ getValue NOTIFY valueChanged)

public:
    explicit HomeDeviceBase(const std::string& id, const std::string& topic, QObject* parent = nullptr) 
        : QObject(parent), id(id), topic(topic), deviceValue(0.0f){}
    virtual ~HomeDeviceBase() = default;
    
    float getValue() const { return deviceValue.get(); }
    std::string getId() const { return id; }
    QString getQStringId() const { return QString::fromStdString(id); }
    QString getQStringTopic() const { return QString::fromStdString(topic); }
    
    virtual DeviceType getType() const = 0; 
    
signals:
    void valueChanged();
    
protected:
    std::string id;
    std::string topic;
    mutable ThreadSafeValue<float> deviceValue;
    
    void updateValue(const float newValue)
    {
        deviceValue.set(newValue);
        
        QMetaObject::invokeMethod(this, [this]
        {
            emit valueChanged();
        }, Qt::QueuedConnection);
    }
};
