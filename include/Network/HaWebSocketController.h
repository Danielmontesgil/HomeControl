#pragma once
#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QTimer>
#include "IHaController.h"

/**
 * @brief Concrete implementation of the Home Assistant controller using Qt WebSockets.
 * Manages the connection lifecycle, HA authentication protocol,
 * initial states dump, and real-time state updates.
 */
class HaWebSocketController : public QObject, public IHaController
{
    Q_OBJECT

public:
    explicit HaWebSocketController(QObject* parent = nullptr);
    ~HaWebSocketController() override;

    // IHaController contract implementation
    void connectToHa(const std::string& url, const std::string& token) override;
    void callService(const std::string& domain, 
                     const std::string& service, 
                     const std::string& entityId, 
                     const QJsonObject& serviceData = {}) override;

    // Testing/state inspection helper
    bool isAuthenticated() const { return m_isAuthenticated; }
    int retryAttemptCount() const { return m_retryAttemptCount; }

    // Constants for Exponential Backoff strategy
    static constexpr int BASE_RETRY_DELAY_MS = 2000;
    static constexpr int MAX_RETRY_DELAY_MS = 30000;
    static constexpr double JITTER_FACTOR = 0.20;

signals:
    /**
     * @brief Emitted when the physical connection and the authentication handshake succeed.
     */
    void connected();

    /**
     * @brief Emitted when the connection is lost or closed.
     */
    void disconnected();

    /**
     * @brief Emitted when a compatible Home Assistant device is discovered.
     * @param type Mapped device type ("Light", "Roller").
     * @param entityId HA entity ID (e.g., "light.living_room_1").
     * @param friendlyName Device friendly name.
     * @param state Current state of the entity.
     * @param attributes Entity attributes (containing brightness, colors, etc.).
     */
    void deviceDiscovered(const QString& type, const QString& entityId, const QString& friendlyName, const QString& state, const QJsonObject& attributes);

    /**
     * @brief Emitted when an existing device changes state in real-time.
     * @param entityId HA entity ID.
     * @param state New state.
     * @param attributes Additional attributes (e.g., brightness, position).
     */
    void deviceStateChanged(const QString& entityId, const QString& state, const QJsonObject& attributes);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);

private:
    QWebSocket m_webSocket;
    std::string m_url;
    std::string m_token;
    QTimer* m_reconnectTimer{nullptr};
    int m_messageId{1};
    bool m_isAuthenticated{false};
    bool m_shouldReconnect{true};
    int m_retryAttemptCount{0};

    void authenticate();
    void subscribeToEvents();
    void fetchInitialStates();
    void parseHaMessage(const QString& message);
    
    // Calculates backoff delay in milliseconds with exponential growth and 20% random jitter
    int calculateNextBackoffDelayMs();
    void resetBackoff();
    
    // Generates a unique sequential message ID for each request
    int nextMessageId() { return m_messageId++; }
};

