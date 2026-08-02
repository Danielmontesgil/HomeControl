#pragma once
#include "ILicenseManager.h"

class ISettingsManager;

class LocalLicenseManager : public ILicenseManager {
public:
    explicit LocalLicenseManager(ISettingsManager& settingsManager);
    ~LocalLicenseManager() override = default;

    bool isFeatureLicensed(const std::string& featureId) const override;
    bool activateFeature(const std::string& featureId, const std::string& licenseKey) override;

private:
    ISettingsManager& m_settingsManager;
};
