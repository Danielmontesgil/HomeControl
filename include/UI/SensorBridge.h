#pragma once
#include <QObject>
#include <QJsonObject>

class DeviceModel;
class IDeviceFactory;
class IHaController;
class ISettingsManager;

class SensorBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(DeviceModel* devices READ getDevices CONSTANT)
    Q_PROPERTY(bool haConnected READ isHaConnected NOTIFY networkChanged)
    Q_PROPERTY(int haLatency READ getHaLatency NOTIFY networkChanged)
    Q_PROPERTY(int haReconnectAttempts READ getHaReconnectAttempts NOTIFY networkChanged)
    Q_PROPERTY(int haNextReconnectDelay READ getHaNextReconnectDelay NOTIFY networkChanged)
    Q_PROPERTY(QString haLastDisconnectReason READ getHaLastDisconnectReason NOTIFY networkChanged)
    Q_PROPERTY(bool isDebugBuild READ isDebugBuild CONSTANT)
    Q_PROPERTY(bool verboseLogging READ isVerboseLoggingEnabled WRITE setVerboseLogging NOTIFY networkChanged)
    
public:
    explicit SensorBridge(IDeviceFactory& deviceFactory, DeviceModel& deviceModel, IHaController& haController, ISettingsManager& settingsManager, QObject* parent = nullptr);
    virtual ~SensorBridge() = default;
    
    Q_INVOKABLE void publishCommand(const QString& topic, const QString& payload);
    Q_INVOKABLE int getDeviceCount(const QString& prefix) const;
    Q_INVOKABLE void addDevice(const QString& type, const QString& id, const QString& topic);
    Q_INVOKABLE int getCountByType(int type) const;
    Q_INVOKABLE void setAllDevicesState(int type, const QString& payload);
    Q_INVOKABLE void stopDevice(const QString& topic);
    Q_INVOKABLE void renameDevice(const QString& topic, const QString& newName);
    
    DeviceModel* getDevices() const { return &m_deviceModel; }

    bool isHaConnected() const;
    int getHaLatency() const;
    int getHaReconnectAttempts() const;
    int getHaNextReconnectDelay() const;
    QString getHaLastDisconnectReason() const;
    bool isDebugBuild() const;
    bool isVerboseLoggingEnabled() const;
    void setVerboseLogging(bool enable);

    Q_INVOKABLE void forceDisconnect();
    Q_INVOKABLE void setSimulationLatency(int ms);
    Q_INVOKABLE void setSimulationAuthFail(bool enable);
    Q_INVOKABLE void setSimulationOfflineMode(bool enable);
    Q_INVOKABLE void reconnect();
    Q_INVOKABLE void copyToClipboard(const QString& text);

    void setHaCredentials(const QString& url, const QString& token);
    Q_INVOKABLE void saveHaCredentials(const QString& url, const QString& token);
    Q_INVOKABLE QString getSavedHaUrl() const;
    Q_INVOKABLE QString getSavedHaToken() const;
    Q_INVOKABLE QString getSavedLanguage() const;
    Q_INVOKABLE void saveLanguage(const QString& lang);
    Q_INVOKABLE QString getHaMapUrl(const QString& entityId) const;

public slots:
    void onDeviceDiscovered(const QString& type, const QString& entityId, const QString& friendlyName, const QString& state, const QJsonObject& attributes);
    void onDeviceStateChanged(const QString& entityId, const QString& state, const QJsonObject& attributes);
    
signals:
    void countChanged();
    void networkChanged();
    void websocketMessageLogged(const QString& direction, const QString& message);
    
private:
    IHaController& m_haController;
    IDeviceFactory& m_deviceFactory;
    DeviceModel& m_deviceModel;
    ISettingsManager& m_settingsManager;
    QString m_haUrl;
    QString m_haToken;
};

