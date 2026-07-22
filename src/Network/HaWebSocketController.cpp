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
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        std::cerr << "[HaWebSocketController] Socket error: " << error << std::endl;
        m_lastDisconnectReason = QString("Socket Error: %1").arg(error);
        emit networkMetricsChanged();
    });

    // Initialize reconnect timer
    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_isAuthenticated && m_shouldReconnect && !m_simulationOfflineMode)
        {
            std::cout << "[HaWebSocketController] Reconnection timer triggered, opening WebSocket..." << std::endl;
            m_webSocket.open(QUrl(QString::fromStdString(m_url)));
        }
    });

    // Initialize ping timer for latency monitoring
    m_pingTimer = new QTimer(this);
    connect(m_pingTimer, &QTimer::timeout, this, [this]() {
        if (m_isAuthenticated)
        {
            int pingId = nextMessageId();
            QJsonObject pingObj;
            pingObj["id"] = pingId;
            pingObj["type"] = "ping";
            
            QJsonDocument doc(pingObj);
            
            QElapsedTimer timer;
            timer.start();
            m_pendingPings[pingId] = timer;
            
            sendTextMessageInternal(doc.toJson(QJsonDocument::Compact));
        }
    });
    m_pingTimer->start(5000);
}

HaWebSocketController::~HaWebSocketController()
{
    m_shouldReconnect = false;
    if (m_reconnectTimer)
    {
        m_reconnectTimer->stop();
    }
    if (m_pingTimer)
    {
        m_pingTimer->stop();
    }
    m_webSocket.close();
}

void HaWebSocketController::connectToHa(const std::string& url, const std::string& token)
{
    m_url = url;
    m_token = token;
    m_isAuthenticated = false;
    m_shouldReconnect = true;
    m_lastDisconnectReason = "None";
    resetBackoff();
    emit networkMetricsChanged();

    if (m_simulationOfflineMode)
    {
        m_lastDisconnectReason = "Offline Mode Simulation Active";
        emit networkMetricsChanged();
        std::cout << "[HaWebSocketController] Connection blocked: Simulation Offline Mode is active." << std::endl;
        return;
    }

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
    sendTextMessageInternal(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::onConnected()
{
    std::cout << "[HaWebSocketController] TCP connection established with the WebSocket." << std::endl;
    if (m_reconnectTimer)
    {
        m_reconnectTimer->stop();
    }
    m_nextReconnectDelayMs = 0;
    emit networkMetricsChanged();
}

void HaWebSocketController::onDisconnected()
{
    std::cout << "[HaWebSocketController] WebSocket connection lost." << std::endl;
    m_isAuthenticated = false;
    m_latencyMs = -1;
    m_pendingPings.clear();

    if (m_lastDisconnectReason == "None")
    {
        m_lastDisconnectReason = "Connection Lost";
    }

    emit disconnected();
    emit networkMetricsChanged();

    if (m_shouldReconnect && m_reconnectTimer && !m_simulationOfflineMode)
    {
        int delayMs = calculateNextBackoffDelayMs();
        std::cout << "[HaWebSocketController] Scheduling reconnect attempt #" << m_retryAttemptCount 
                  << " in " << delayMs << " ms." << std::endl;
        m_nextReconnectDelayMs = delayMs;
        m_reconnectTimerStart.start();
        m_reconnectTimer->start(delayMs);
        emit networkMetricsChanged();
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
    const bool isPong = message.contains("\"type\":\"pong\"") || message.contains("\"type\": \"pong\"");
    if (m_verboseLogging && !isPong)
    {
        std::cout << "[WS RCVD] " << message.toStdString() << std::endl;
    }

    if (!isPong)
    {
        emit messageLogged("in", message);
    }
    
    if (m_simulationLatency > 0)
    {
        QTimer::singleShot(m_simulationLatency, this, [this, message]() {
            parseHaMessage(message);
        });
    }
    else
    {
        parseHaMessage(message);
    }
}

void HaWebSocketController::authenticate()
{
    std::cout << "[HaWebSocketController] Sending access token to HA..." << std::endl;
    
    QJsonObject authObj;
    authObj["type"] = "auth";
    if (m_simulationAuthFail)
    {
        authObj["access_token"] = "simulated_invalid_token_12345";
        std::cout << "[HaWebSocketController] Simulating Auth Failure: sending invalid token." << std::endl;
    }
    else
    {
        authObj["access_token"] = QString::fromStdString(m_token);
    }

    QJsonDocument doc(authObj);
    sendTextMessageInternal(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::subscribeToEvents()
{
    std::cout << "[HaWebSocketController] Subscribing to state changes..." << std::endl;

    QJsonObject subObj;
    subObj["id"] = nextMessageId();
    subObj["type"] = "subscribe_events";
    subObj["event_type"] = "state_changed";

    QJsonDocument doc(subObj);
    sendTextMessageInternal(doc.toJson(QJsonDocument::Compact));
}

void HaWebSocketController::fetchInitialStates()
{
    std::cout << "[HaWebSocketController] Requesting initial states dump..." << std::endl;

    QJsonObject statesObj;
    statesObj["id"] = nextMessageId();
    statesObj["type"] = "get_states";

    QJsonDocument doc(statesObj);
    sendTextMessageInternal(doc.toJson(QJsonDocument::Compact));
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
        m_lastDisconnectReason = "None";
        resetBackoff();
        emit connected();
        emit networkMetricsChanged();

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
        m_lastDisconnectReason = "Authentication Failed: Invalid Token";
        emit networkMetricsChanged();
        m_webSocket.close();
    }
    else if (msgType == "pong")
    {
        int id = rootObj["id"].toInt();
        auto it = m_pendingPings.find(id);
        if (it != m_pendingPings.end())
        {
            m_latencyMs = static_cast<int>(it->second.elapsed());
            m_pendingPings.erase(it);
            emit networkMetricsChanged();
        }
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

bool HaWebSocketController::isConnected() const
{
    return m_isAuthenticated;
}

int HaWebSocketController::getLatencyMs() const
{
    return m_latencyMs;
}

int HaWebSocketController::getReconnectAttempts() const
{
    return m_retryAttemptCount;
}

int HaWebSocketController::getNextReconnectDelayMs() const
{
    if (m_reconnectTimer && m_reconnectTimer->isActive())
    {
        qint64 elapsed = m_reconnectTimerStart.elapsed();
        qint64 remaining = m_nextReconnectDelayMs - elapsed;
        return std::max(0, static_cast<int>(remaining));
    }
    return 0;
}

std::string HaWebSocketController::getLastDisconnectReason() const
{
    return m_lastDisconnectReason.toStdString();
}

void HaWebSocketController::forceDisconnect()
{
    std::cout << "[HaWebSocketController] Force Disconnect triggered by developer." << std::endl;
    m_lastDisconnectReason = "Forced Disconnect (DevTools)";
    emit networkMetricsChanged();
    m_webSocket.close();
}

void HaWebSocketController::setSimulationLatency(int ms)
{
    m_simulationLatency = ms;
    std::cout << "[HaWebSocketController] Simulation latency set to " << ms << " ms." << std::endl;
    emit networkMetricsChanged();
}

void HaWebSocketController::setSimulationAuthFail(bool enable)
{
    m_simulationAuthFail = enable;
    std::cout << "[HaWebSocketController] Simulation Auth Fail set to " << (enable ? "true" : "false") << std::endl;
    emit networkMetricsChanged();
}

void HaWebSocketController::setSimulationOfflineMode(bool enable)
{
    m_simulationOfflineMode = enable;
    std::cout << "[HaWebSocketController] Simulation Offline Mode set to " << (enable ? "true" : "false") << std::endl;
    if (enable)
    {
        if (m_reconnectTimer)
        {
            m_reconnectTimer->stop();
        }
        m_lastDisconnectReason = "Offline Mode Simulation Active";
        m_webSocket.close();
    }
    emit networkMetricsChanged();
}

void HaWebSocketController::sendTextMessageInternal(const QString& message)
{
    const bool isPing = message.contains("\"type\":\"ping\"") || message.contains("\"type\": \"ping\"");
    if (m_verboseLogging && !isPing)
    {
        std::cout << "[WS SENT] " << message.toStdString() << std::endl;
    }

    if (!isPing)
    {
        emit messageLogged("out", message);
    }
    
    if (m_simulationLatency > 0)
    {
        QTimer::singleShot(m_simulationLatency, this, [this, message]() {
            m_webSocket.sendTextMessage(message);
        });
    }
    else
    {
        m_webSocket.sendTextMessage(message);
    }
}

bool HaWebSocketController::isVerboseLoggingEnabled() const
{
    return m_verboseLogging;
}

void HaWebSocketController::setVerboseLogging(bool enable)
{
    m_verboseLogging = enable;
    emit networkMetricsChanged();
}
