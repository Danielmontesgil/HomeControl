#include "DeviceMappingManager.h"
#include "Core/Log.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

bool DeviceMappingManager::loadMappings(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly))
    {
        Log::warn("DeviceMappingManager", "Could not open config file for mappings: " + filepath.toStdString());
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        Log::error("DeviceMappingManager", "Error parsing config JSON: " + parseError.errorString().toStdString());
        return false;
    }

    loadFromJsonObject(doc.object());
    return true;
}

void DeviceMappingManager::loadFromJsonObject(const QJsonObject& rootObj)
{
    m_devices.clear();
    m_entityToDeviceTopic.clear();

    if (!rootObj.contains("compound_devices") || !rootObj["compound_devices"].isArray())
    {
        Log::info("DeviceMappingManager", "No compound_devices array found in configuration.");
        return;
    }

    QJsonArray compoundDevices = rootObj["compound_devices"].toArray();
    for (const QJsonValue& deviceVal : compoundDevices)
    {
        QJsonObject deviceObj = deviceVal.toObject();
        QString id = deviceObj["id"].toString();
        QString name = deviceObj["name"].toString();
        QString type = deviceObj["type"].toString();

        if (id.isEmpty() || type.isEmpty())
        {
            Log::warn("DeviceMappingManager", "Invalid compound device entry skipped. Missing 'id' or 'type'.");
            continue;
        }

        CompoundDeviceConfig config;
        config.topic = id;
        config.name = name.isEmpty() ? id : name;
        config.type = type;

        if (deviceObj.contains("entities") && deviceObj["entities"].isObject())
        {
            QJsonObject entitiesObj = deviceObj["entities"].toObject();
            for (auto it = entitiesObj.begin(); it != entitiesObj.end(); ++it)
            {
                QString role = it.key();
                QString entityId = it.value().toString();
                if (!entityId.isEmpty())
                {
                    config.entities.insert(role, entityId);
                    m_entityToDeviceTopic.insert(entityId.toLower(), id);
                }
            }
        }

        m_devices.insert(id, config);
        Log::info("DeviceMappingManager", "Registered compound device mapping: " + id.toStdString() + " of type " + type.toStdString());
    }
}

bool DeviceMappingManager::isEntityMapped(const QString& entityId) const
{
    return m_entityToDeviceTopic.contains(entityId.toLower());
}

QString DeviceMappingManager::getDeviceTopicForEntity(const QString& entityId) const
{
    return m_entityToDeviceTopic.value(entityId.toLower());
}

QString DeviceMappingManager::getDeviceTypeForEntity(const QString& entityId) const
{
    QString topic = getDeviceTopicForEntity(entityId);
    if (!topic.isEmpty())
    {
        return m_devices.value(topic).type;
    }
    return QString();
}

QString DeviceMappingManager::getDeviceNameForEntity(const QString& entityId) const
{
    QString topic = getDeviceTopicForEntity(entityId);
    if (!topic.isEmpty())
    {
        return m_devices.value(topic).name;
    }
    return QString();
}

QVariantMap DeviceMappingManager::getEntityMappingsForDevice(const QString& deviceTopic) const
{
    if (m_devices.contains(deviceTopic))
    {
        return m_devices.value(deviceTopic).entities;
    }
    return QVariantMap();
}
