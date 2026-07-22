#include "JsonSettingsManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThreadPool>
#include <QRunnable>
#include <iostream>

JsonSettingsManager::JsonSettingsManager(const std::string& filepath)
    : m_filepath(filepath)
{
    load();
}

JsonSettingsManager::~JsonSettingsManager()
{
    QThreadPool::globalInstance()->waitForDone();
}

void JsonSettingsManager::saveAlias(const std::string& entityId, const std::string& alias)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aliases[entityId] = alias;
    save();
}

std::string JsonSettingsManager::getAlias(const std::string& entityId, const std::string& defaultAlias)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_aliases.find(entityId);
    if (it != m_aliases.end())
    {
        return it->second;
    }
    return defaultAlias;
}

void JsonSettingsManager::saveVisibility(const std::string& entityId, bool visible)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_visibilities[entityId] = visible;
    save();
}

bool JsonSettingsManager::getVisibility(const std::string& entityId, bool defaultVisible)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_visibilities.find(entityId);
    if (it != m_visibilities.end())
    {
        return it->second;
    }
    return defaultVisible;
}

void JsonSettingsManager::load()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    QFile file(QString::fromStdString(m_filepath));
    if (!file.open(QIODevice::ReadOnly))
    {
        return; // File doesn't exist yet, which is fine
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        std::cerr << "[JsonSettingsManager] JSON parse error: " 
                  << parseError.errorString().toStdString() << std::endl;
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject devices = root["devices"].toObject();

    for (auto it = devices.begin(); it != devices.end(); ++it)
    {
        std::string entityId = it.key().toStdString();
        QJsonObject devObj = it.value().toObject();

        if (devObj.contains("alias"))
        {
            m_aliases[entityId] = devObj["alias"].toString().toStdString();
        }
        if (devObj.contains("visible"))
        {
            m_visibilities[entityId] = devObj["visible"].toBool();
        }
    }
}

void JsonSettingsManager::save()
{
    QJsonObject root;
    QJsonObject devicesObj;
    std::unordered_map<std::string, QJsonObject> devConfigs;

    for (const auto& [entityId, alias] : m_aliases)
    {
        devConfigs[entityId]["alias"] = QString::fromStdString(alias);
    }

    for (const auto& [entityId, visible] : m_visibilities)
    {
        devConfigs[entityId]["visible"] = visible;
    }

    for (const auto& [entityId, config] : devConfigs)
    {
        devicesObj[QString::fromStdString(entityId)] = config;
    }

    root["devices"] = devicesObj;

    const std::string filepath = m_filepath;

    QThreadPool::globalInstance()->start(QRunnable::create([this, filepath, root]() {
        std::lock_guard<std::mutex> fileLock(m_writeMutex);
        QFile file(QString::fromStdString(filepath));
        if (!file.open(QIODevice::WriteOnly))
        {
            std::cerr << "[JsonSettingsManager] Error: Unable to open file for writing: " 
                      << filepath << std::endl;
            return;
        }

        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }));
}
