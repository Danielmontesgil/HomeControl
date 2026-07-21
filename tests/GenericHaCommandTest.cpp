#include <gtest/gtest.h>
#include "Commands/GenericHaCommand.h"
#include "Network/IHaController.h"
#include <QJsonObject>
#include <QJsonArray>
#include <string>

namespace {

class MockHaController : public IHaController {
public:
    void connectToHa(const std::string& url, const std::string& token) override {
        m_connected = true;
        m_url = url;
        m_token = token;
    }

    void callService(const std::string& domain,
                     const std::string& service,
                     const std::string& entityId,
                     const QJsonObject& serviceData = {}) override {
        m_callServiceCalled = true;
        m_lastDomain = domain;
        m_lastService = service;
        m_lastEntityId = entityId;
        m_lastServiceData = serviceData;
    }

    bool isConnected() const { return m_connected; }
    const std::string& getUrl() const { return m_url; }
    const std::string& getToken() const { return m_token; }

    bool wasCallServiceCalled() const { return m_callServiceCalled; }
    const std::string& getLastDomain() const { return m_lastDomain; }
    const std::string& getLastService() const { return m_lastService; }
    const std::string& getLastEntityId() const { return m_lastEntityId; }
    const QJsonObject& getLastServiceData() const { return m_lastServiceData; }

    void reset() {
        m_connected = false;
        m_url.clear();
        m_token.clear();
        m_callServiceCalled = false;
        m_lastDomain.clear();
        m_lastService.clear();
        m_lastEntityId.clear();
        m_lastServiceData = QJsonObject();
    }

private:
    bool m_connected = false;
    std::string m_url;
    std::string m_token;
    bool m_callServiceCalled = false;
    std::string m_lastDomain;
    std::string m_lastService;
    std::string m_lastEntityId;
    QJsonObject m_lastServiceData;
};

} // namespace

TEST(GenericHaCommandTest, ExecuteSimpleCommandInvokesCallService) {
    MockHaController mockController;
    const std::string expectedDomain = "light";
    const std::string expectedService = "turn_on";
    const std::string expectedEntityId = "light.living_room_light";

    GenericHaCommand command(mockController, expectedDomain, expectedService, expectedEntityId);
    command.execute();

    EXPECT_TRUE(mockController.wasCallServiceCalled());
    EXPECT_EQ(mockController.getLastDomain(), expectedDomain);
    EXPECT_EQ(mockController.getLastService(), expectedService);
    EXPECT_EQ(mockController.getLastEntityId(), expectedEntityId);
    EXPECT_TRUE(mockController.getLastServiceData().isEmpty());
}

TEST(GenericHaCommandTest, ExecuteCommandWithDataTransmitsData) {
    MockHaController mockController;
    const std::string expectedDomain = "light";
    const std::string expectedService = "turn_on";
    const std::string expectedEntityId = "light.living_room_light";

    QJsonObject expectedData;
    expectedData["brightness"] = 255;
    expectedData["rgb_color"] = QJsonArray({255, 0, 0});

    GenericHaCommand command(mockController, expectedDomain, expectedService, expectedEntityId, expectedData);
    command.execute();

    EXPECT_TRUE(mockController.wasCallServiceCalled());
    EXPECT_EQ(mockController.getLastDomain(), expectedDomain);
    EXPECT_EQ(mockController.getLastService(), expectedService);
    EXPECT_EQ(mockController.getLastEntityId(), expectedEntityId);
    
    QJsonObject actualData = mockController.getLastServiceData();
    EXPECT_FALSE(actualData.isEmpty());
    EXPECT_EQ(actualData["brightness"].toInt(), 255);
    ASSERT_TRUE(actualData["rgb_color"].isArray());
    
    QJsonArray rgbArray = actualData["rgb_color"].toArray();
    ASSERT_EQ(rgbArray.size(), 3);
    EXPECT_EQ(rgbArray[0].toInt(), 255);
    EXPECT_EQ(rgbArray[1].toInt(), 0);
    EXPECT_EQ(rgbArray[2].toInt(), 0);
}
