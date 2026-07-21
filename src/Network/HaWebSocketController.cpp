#include "HaWebSocketController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

HaWebSocketController::HaWebSocketController(QObject* parent)
    : QObject(parent)
{
    // Connect QWebSocket signals with the corresponding controller slots.
    // This asynchronously handles network events on the Qt Event Loop.
    connect(&m_webSocket, &QWebSocket::connected, this, &HaWebSocketController::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &HaWebSocketController::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &HaWebSocketController::onTextMessageReceived);
    
    // Catch network errors for console diagnostics
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, [](QAbstractSocket::SocketError error) {
        std::cerr << "[HaWebSocketController] Socket error: " << error << std::endl;
    });

    // Initialize reconnect timer
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_isAuthenticated && m_shouldReconnect)
        {
            std::cout << "[HaWebSocketController] Reconnection timer triggered, opening WebSocket..." << std::endl;
            m_webSocket.open(QUrl(QString::fromStdString(m_url)));
        }
    });
}

HaWebSocketController::~HaWebSocketController()
{
    m_shouldReconnect = false;
    if (m_reconnectTimer)
    {
        m_reconnectTimer->stop();
    }
    m_webSocket.close();
}

void HaWebSocketController::connectToHa(const std::string& url, const std::string& token)
{
    m_url = url;
    m_token = token;
    m_isAuthenticated = false;
    m_shouldReconnect = true;
    resetBackoff();

    std::cout << "[HaWebSocketController] Connecting to " << m_url << "..." << std::endl;
    m_webSocket.open(QUrl(QString::fromStdString(m_url)));
}

void HaWebSocketController::callService(const std::string& domain, 
                                       const std::string& service, 
                                       const std::string& entityId, 
                                       const QJsonObject& serviceData)
{
    if (!m_isAuthenticated)
    {
        std::cerr << "[HaWebSocketController] Warning: Attempting to call a service without authentication." << std::endl;
        return;
    }

    // Structure the service call request according to the Home Assistant specification:
    // https://developers.home-assistant.io/docs/api/websocket/#calling-a-service
    QJsonObject requestObj;
    requestObj["id"] = nextMessageId();
    requestObj["type"] = "call_service";
    requestObj["domain"] = QString::fromStdString(domain);
    requestObj["service"] = QString::fromStdString(service);

    QJsonObject targetData = serviceData;
    targetData["entity_id"] = QString::fromStdString(entityId);
    requestObj["service_data"] = targetData;

    QJsonDocument doc(requestObj);
    m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::onConnected()
{
    std::cout << "[HaWebSocketController] TCP connection established with the WebSocket." << std::endl;
    if (m_reconnectTimer)
    {
        m_reconnectTimer->stop();
    }
}

void HaWebSocketController::onDisconnected()
{
    std::cout << "[HaWebSocketController] WebSocket connection lost." << std::endl;
    m_isAuthenticated = false;
    emit disconnected();
    if (m_shouldReconnect && m_reconnectTimer)
    {
        int delayMs = calculateNextBackoffDelayMs();
        std::cout << "[HaWebSocketController] Scheduling reconnect attempt #" << m_retryAttemptCount 
                  << " in " << delayMs << " ms." << std::endl;
        m_reconnectTimer->start(delayMs);
    }
}

int HaWebSocketController::calculateNextBackoffDelayMs()
{
    // Exponential formula: base * 2^attempt
    double exponentialDelay = BASE_RETRY_DELAY_MS * std::pow(2.0, m_retryAttemptCount);
    double cappedDelay = std::min(exponentialDelay, static_cast<double>(MAX_RETRY_DELAY_MS));

    // Jitter calculation (+/- 20%)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(1.0 - JITTER_FACTOR, 1.0 + JITTER_FACTOR);

    double jitteredDelay = cappedDelay * dis(gen);

    if (m_retryAttemptCount < 10)
    {
        m_retryAttemptCount++;
    }

    return static_cast<int>(jitteredDelay);
}

void HaWebSocketController::resetBackoff()
{
    m_retryAttemptCount = 0;
}

void HaWebSocketController::onTextMessageReceived(const QString& message)
{
    parseHaMessage(message);
}

void HaWebSocketController::authenticate()
{
    std::cout << "[HaWebSocketController] Sending access token to HA..." << std::endl;
    
    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["access_token"] = QString::fromStdString(m_token);

    QJsonDocument doc(authObj);
    m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::subscribeToEvents()
{
    std::cout << "[HaWebSocketController] Subscribing to state changes..." << std::endl;

    QJsonObject subObj;
    subObj["id"] = nextMessageId();
    subObj["type"] = "subscribe_events";
    subObj["event_type"] = "state_changed";

    QJsonDocument doc(subObj);
    m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::fetchInitialStates()
{
    std::cout << "[HaWebSocketController] Requesting initial states dump..." << std::endl;

    QJsonObject statesObj;
    statesObj["id"] = nextMessageId();
    statesObj["type"] = "get_states";

    QJsonDocument doc(statesObj);
    m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::parseHaMessage(const QString& message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        std::cerr << "[HaWebSocketController] Error parsing received JSON: " 
                  << parseError.errorString().toStdString() << std::endl;
        return;
    }

    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("type") || !rootObj["type"].isString())
    {
        std::cerr << "[HaWebSocketController] Error: Received JSON payload lacks a valid string 'type' key." << std::endl;
        return;
    }
    QString msgType = rootObj["type"].toString();

    // HA Handshake and Authentication Flow
    if (msgType == "auth_required")
    {
        authenticate();
    }
    else if (msgType == "auth_ok")
    {
        std::cout << "[HaWebSocketController] Authentication successfully completed." << std::endl;
        m_isAuthenticated = true;
        resetBackoff();
        emit connected();

        // After successful authentication, sync states and subscribe to real-time events
        fetchInitialStates();
        subscribeToEvents();
    }
    else if (msgType == "auth_invalid")
    {
        std::cerr << "[HaWebSocketController] ERROR: Invalid access token for Home Assistant: " 
                  << rootObj["message"].toString().toStdString() << std::endl;
        m_shouldReconnect = false;
        if (m_reconnectTimer)
        {
            m_reconnectTimer->stop();
        }
        resetBackoff();
        m_webSocket.close();
    }
    // Responses to sent commands (like get_states)
    else if (msgType == "result")
    {
        bool success = rootObj["success"].toBool();
        if (!success)
        {
            QJsonObject errorObj = rootObj["error"].toObject();
            std::cerr << "[HaWebSocketController] Error returned by HA. Code: " 
                      << errorObj["code"].toString().toStdString() << " Msg: "
                      << errorObj["message"].toString().toStdString() << std::endl;
            return;
        }

        // If the "result" is a list (get_states response), process existing devices
        QJsonValue resultValue = rootObj["result"];
        if (resultValue.isArray())
        {
            QJsonArray statesArray = resultValue.toArray();
            for (const QJsonValue& val : statesArray)
            {
                QJsonObject entityObj = val.toObject();
                QString entityId = entityObj["entity_id"].toString();
                QString state = entityObj["state"].toString();
                QJsonObject attributes = entityObj["attributes"].toObject();
                QString friendlyName = attributes["friendly_name"].toString(entityId);

                // Map target entity types
                QString type;
                if (entityId.startsWith("light."))
                {
                    // Check if it is a smart light with dimming and color capabilities
                    type = "Light";
                    if (attributes.contains("supported_color_modes"))
                    {
                        QJsonArray modes = attributes["supported_color_modes"].toArray();
                        bool hasColor = false;
                        for (const auto& modeVal : modes)
                        {
                            QString mode = modeVal.toString();
                            if (mode == "color_temp" || mode == "hs" || mode == "rgb" || mode == "rgbw" || mode == "xy")
                            {
                                hasColor = true;
                                break;
                            }
                        }
                        if (hasColor)
                        {
                            type = "DimmableColorLight";
                        }
                    }
                }
                else if (entityId.startsWith("cover."))
                {
                    type = "Roller";
                }
                else if (entityId.startsWith("vacuum."))
                {
                    type = "Vacuum";
                }

                if (!type.isEmpty())
                {
                    emit deviceDiscovered(type, entityId, friendlyName, state, attributes);
                }
            }
        }
    }
    // Real-time events sent by HA
    else if (msgType == "event")
    {
        QJsonObject eventObj = rootObj["event"].toObject();
        QString eventType = eventObj["event_type"].toString();

        if (eventType == "state_changed")
        {
            QJsonObject dataObj = eventObj["data"].toObject();
            QString entityId = dataObj["entity_id"].toString();
            QJsonObject newStateObj = dataObj["new_state"].toObject();
            
            // If the device was deleted or deactivated, its new_state might be null
            if (!newStateObj.isEmpty())
            {
                QString state = newStateObj["state"].toString();
                QJsonObject attributes = newStateObj["attributes"].toObject();

                // Notify entity state change
                emit deviceStateChanged(entityId, state, attributes);
            }
        }
    }
}
