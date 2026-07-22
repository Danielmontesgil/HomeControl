#pragma once
#include "ISettingsManager.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <QString>
#include <QJsonObject>

class JsonSettingsManager : public ISettingsManager
{
public:
    explicit JsonSettingsManager(const std::string& filepath);
    ~JsonSettingsManager() override;

    void saveAlias(const std::string& entityId, const std::string& alias) override;
    std::string getAlias(const std::string& entityId, const std::string& defaultAlias) override;

    void saveVisibility(const std::string& entityId, bool visible) override;
    bool getVisibility(const std::string& entityId, bool defaultVisible) override;

private:
    std::string m_filepath;
    std::unordered_map<std::string, std::string> m_aliases;
    std::unordered_map<std::string, bool> m_visibilities;
    std::mutex m_mutex;
    std::mutex m_writeMutex;

    void load();
    void save();
};
