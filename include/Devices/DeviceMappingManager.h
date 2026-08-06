#pragma once
#include <QString>
#include <QHash>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>

struct CompoundDeviceConfig
{
    QString topic;
    QString name;
    QString type;
    QVariantMap entities; // mapping of: role -> entityId
};

class DeviceMappingManager
{
public:
    DeviceMappingManager() = default;
    ~DeviceMappingManager() = default;

    bool loadMappings(const QString& filepath);
    void loadFromJsonObject(const QJsonObject& rootObj);

    bool isEntityMapped(const QString& entityId) const;
    QString getDeviceTopicForEntity(const QString& entityId) const;
    QString getDeviceTypeForEntity(const QString& entityId) const;
    QString getDeviceNameForEntity(const QString& entityId) const;
    QVariantMap getEntityMappingsForDevice(const QString& deviceTopic) const;

private:
    QHash<QString, CompoundDeviceConfig> m_devices; // deviceTopic -> config
    QHash<QString, QString> m_entityToDeviceTopic;  // entityId -> deviceTopic
};
