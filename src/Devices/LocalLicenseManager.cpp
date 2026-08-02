#include "LocalLicenseManager.h"
#include "ISettingsManager.h"

LocalLicenseManager::LocalLicenseManager(ISettingsManager& settingsManager)
    : m_settingsManager(settingsManager)
{
}

bool LocalLicenseManager::isFeatureLicensed(const std::string& featureId) const
{
    if (featureId == "parental") {
        std::string savedKey = m_settingsManager.getAlias("system.parental_license", "");
        return (savedKey == "DEMO-PARENTAL-KEY-2026");
    }
    return false;
}

bool LocalLicenseManager::activateFeature(const std::string& featureId, const std::string& licenseKey)
{
    if (featureId == "parental") {
        if (licenseKey == "DEMO-PARENTAL-KEY-2026" || licenseKey.empty()) {
            m_settingsManager.saveAlias("system.parental_license", licenseKey);
            return true;
        }
    }
    return false;
}
