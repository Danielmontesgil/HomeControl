#pragma once
#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QSet>

struct BlockedDevice {
    QString name;
    QString entityId;
    QString type;
    bool isActive;

    BlockedDevice(const QString& n, const QString& e, const QString& t, bool active)
        : name(n), entityId(e), type(t), isActive(active) {}
};

class InternetAccessModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int activeCount READ activeCount NOTIFY activeCountChanged)

public:
    enum InternetAccessRoles {
        NameRole = Qt::UserRole + 1,
        EntityIdRole,
        TypeRole,
        IsActiveRole
    };
    Q_ENUM(InternetAccessRoles)

    explicit InternetAccessModel(QObject* parent = nullptr);
    ~InternetAccessModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int activeCount() const;
    void clear();

public slots:
    void handleSwitchUpdate(const QString& entityId, const QString& state, const QString& friendlyName);
    void handleTrackerUpdate(const QString& entityId, const QString& state, const QString& friendlyName);

signals:
    void activeCountChanged();

private:
    QSet<QString> m_onlineDevices;
    QVector<BlockedDevice> m_devices;

    int findIndexByEntityId(const QString& entityId) const;
    bool isAllowedDevice(const QString& entityId, const QString& name) const;
    QString determineType(const QString& entityId, const QString& name) const;
    QString extractDeviceKey(const QString& entityId) const;
    QVector<BlockedDevice> getFilteredDevices() const;
};
