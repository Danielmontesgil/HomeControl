#include "MqttController.h"
#include "IMqttListener.h"
#include <iostream>

MqttController::MqttController(const std::string& serverAddress, const std::string& clientId) : m_client(serverAddress, clientId) 
{
    m_client.set_callback(*this);
}

void MqttController::message_arrived(const mqtt::const_message_ptr message)
{
    const auto topic = message->get_topic();
    const auto payload = message->get_payload_str();
    
    for (const auto listener : m_globalListeners)
    {
        listener->onMessageReceived(topic, payload);
    }
    
    auto pair = m_listeners.lower_bound(topic);
    while (pair != m_listeners.end() && pair->first.starts_with(topic))
    {
        for (const auto listener : pair->second)
        {
            listener->onMessageReceived(topic, payload);
        }
        ++pair;
    }
}

void MqttController::connect() {
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);
    try {
        m_client.connect(connOpts)->wait(); // Conexión síncrona por ahora
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error conectando a MQTT: " << exc.what() << std::endl;
    }
}

void MqttController::publish(const std::string& topic, const std::string& payload)
{
    m_client.publish(topic, payload);
}

void MqttController::subscribe(const std::string& topic)
{
    m_client.subscribe(topic, 1);
}

void MqttController::addListener(const std::string& topic, IMqttListener* listener)
{
    if (topic == "*")
    {
        m_globalListeners.push_back(listener);
        return;
    }
    m_listeners[topic].push_back(listener);
}

std::vector<std::string> MqttController::getRegisteredTopics() const
{
    std::vector<std::string> topics;
    topics.reserve(m_listeners.size());
    for (const auto& [topic, listeners] : m_listeners)
    {
        topics.push_back(topic);
    }
    return topics;
}
