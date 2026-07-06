#pragma once
#include <string>

class ISettingsManager
{
public:
    virtual ~ISettingsManager() = default;
    
    virtual void saveAlias(const std::string& entityId, const std::string& alias) = 0;
    virtual std::string getAlias(const std::string& entityId, const std::string& defaultAlias) = 0;
    
    virtual void saveVisibility(const std::string& entityId, bool visible) = 0;
    virtual bool getVisibility(const std::string& entityId, bool defaultVisible) = 0;
};
