#include <gtest/gtest.h>
#include "Devices/LocalLicenseManager.h"
#include "ISettingsManager.h"
#include <map>
#include <string>

class MockSettingsManager : public ISettingsManager {
public:
    std::map<std::string, std::string> aliases;
    std::map<std::string, bool> visibility;

    void saveAlias(const std::string& entityId, const std::string& alias) override {
        aliases[entityId] = alias;
    }
    std::string getAlias(const std::string& entityId, const std::string& defaultAlias) override {
        auto it = aliases.find(entityId);
        if (it != aliases.end()) {
            return it->second;
        }
        return defaultAlias;
    }
    void saveVisibility(const std::string& entityId, bool visible) override {
        visibility[entityId] = visible;
    }
    bool getVisibility(const std::string& entityId, bool defaultVisible) override {
        auto it = visibility.find(entityId);
        if (it != visibility.end()) {
            return it->second;
        }
        return defaultVisible;
    }
};

TEST(LocalLicenseManagerTest, ParentalLicenseValidationFlow) {
    MockSettingsManager mockSettings;
    LocalLicenseManager manager(mockSettings);

    // Initial state: license should not be active
    EXPECT_FALSE(manager.isFeatureLicensed("parental"));
    EXPECT_FALSE(manager.isFeatureLicensed("unsupported_feature"));

    // Activate with invalid key
    EXPECT_FALSE(manager.activateFeature("parental", "INVALID-KEY"));
    EXPECT_FALSE(manager.isFeatureLicensed("parental"));

    // Activate with valid key
    EXPECT_TRUE(manager.activateFeature("parental", "DEMO-PARENTAL-KEY-2026"));
    EXPECT_TRUE(manager.isFeatureLicensed("parental"));
    EXPECT_EQ(mockSettings.getAlias("system.parental_license", ""), "DEMO-PARENTAL-KEY-2026");

    // Deactivate by passing empty key
    EXPECT_TRUE(manager.activateFeature("parental", ""));
    EXPECT_FALSE(manager.isFeatureLicensed("parental"));
    EXPECT_EQ(mockSettings.getAlias("system.parental_license", ""), "");

    // Test activation of other features should fail
    EXPECT_FALSE(manager.activateFeature("some_other_feature", "KEY"));
}
