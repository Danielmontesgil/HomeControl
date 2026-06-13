#pragma once
#include <map>
#include <vector>
#include "IMqttController.h"
#include "mqtt/async_client.h"
#include "IMqttListener.h"

class MqttController : public IMqttController, public mqtt::callback
{
public:
    MqttController(const std::string& serverAddress, const std::string& clientId);
    ~MqttController() override = default;
    
    void message_arrived(mqtt::const_message_ptr message) override;
    
    void connect() override;
    void publish(const std::string& topic, const std::string& payload) override;
    void subscribe(const std::string& topic) override;
    void addListener(const std::string& topic, IMqttListener* listener) override;
    
private:
    mqtt::async_client m_client;
    std::map<std::string, std::vector<IMqttListener*>> m_listeners;
    std::vector<IMqttListener*> m_globalListeners;
};
