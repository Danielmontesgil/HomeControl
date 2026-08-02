#include "InternetAccessModel.h"

InternetAccessModel::InternetAccessModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int InternetAccessModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return getFilteredDevices().size();
}

QVariant InternetAccessModel::data(const QModelIndex& index, int role) const
{
    auto filtered = getFilteredDevices();
    if (!index.isValid() || index.row() < 0 || index.row() >= filtered.size()) {
        return QVariant();
    }

    const auto& device = filtered[index.row()];
    switch (role) {
    case NameRole:
        return device.name;
    case EntityIdRole:
        return device.entityId;
    case TypeRole:
        return device.type;
    case IsActiveRole:
        return device.isActive;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> InternetAccessModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[EntityIdRole] = "entityId";
    roles[TypeRole] = "type";
    roles[IsActiveRole] = "isActive";
    return roles;
}

int InternetAccessModel::activeCount() const
{
    return getFilteredDevices().size();
}

void InternetAccessModel::handleSwitchUpdate(const QString& entityId, const QString& state, const QString& friendlyName)
{
    if (!isAllowedDevice(entityId, friendlyName)) {
        return;
    }

    int index = findIndexByEntityId(entityId);
    bool isActive = (state.toLower() == "on");

    if (index == -1) {
        beginResetModel();
        QString type = determineType(entityId, friendlyName);
        m_devices.emplaceBack(friendlyName, entityId, type, isActive);
        endResetModel();
        emit activeCountChanged();
    } else {
        if (m_devices[index].isActive != isActive) {
            m_devices[index].isActive = isActive;
            
            // Buscar la fila correspondiente en el vector filtrado para notificar a la UI
            auto filtered = getFilteredDevices();
            int filteredIdx = -1;
            for (int i = 0; i < filtered.size(); ++i) {
                if (filtered[i].entityId == entityId) {
                    filteredIdx = i;
                    break;
                }
            }
            
            if (filteredIdx != -1) {
                QModelIndex qmlIdx = createIndex(filteredIdx, 0);
                emit dataChanged(qmlIdx, qmlIdx, {IsActiveRole});
            }
        }
    }
}

void InternetAccessModel::handleTrackerUpdate(const QString& entityId, const QString& state, const QString& friendlyName)
{
    QString key = extractDeviceKey(entityId);
    bool isHome = (state.toLower() == QStringLiteral("home"));
    bool wasOnline = m_onlineDevices.contains(key);
    
    if (isHome && !wasOnline) {
        beginResetModel();
        m_onlineDevices.insert(key);
        endResetModel();
        emit activeCountChanged();
    } else if (!isHome && wasOnline) {
        beginResetModel();
        m_onlineDevices.remove(key);
        endResetModel();
        emit activeCountChanged();
    }
}

int InternetAccessModel::findIndexByEntityId(const QString& entityId) const
{
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].entityId == entityId) {
            return i;
        }
    }
    return -1;
}

bool InternetAccessModel::isAllowedDevice(const QString& entityId, const QString& name) const
{
    QString idLower = entityId.toLower();
    QString nameLower = name.toLower();

    // Descartar explícitamente domótica y electrodomésticos para evitar contaminación visual
    if (idLower.contains("washer") || nameLower.contains("washer") ||
        idLower.contains("vacuum") || nameLower.contains("vacuum") ||
        idLower.contains("dishwasher") || nameLower.contains("dishwasher") ||
        idLower.contains("light") || nameLower.contains("light") ||
        idLower.contains("cover") || nameLower.contains("cover") ||
        idLower.contains("dimmer") || nameLower.contains("dimmer") ||
        idLower.contains("sensor") || nameLower.contains("sensor")) 
    {
        return false;
    }

    // Permitir explícitamente dispositivos de entretenimiento, comunicación y ocio
    if (idLower.contains("tv") || nameLower.contains("tv") || nameLower.contains("television") ||
        idLower.contains("phone") || nameLower.contains("phone") ||
        idLower.contains("mobile") || nameLower.contains("mobile") ||
        idLower.contains("playstation") || nameLower.contains("playstation") ||
        idLower.contains("xbox") || nameLower.contains("xbox") ||
        idLower.contains("nintendo") || nameLower.contains("nintendo") ||
        idLower.contains("switch") || nameLower.contains("switch") ||
        idLower.contains("pc") || nameLower.contains("pc") ||
        idLower.contains("computer") || nameLower.contains("computer") ||
        idLower.contains("ipad") || nameLower.contains("ipad") ||
        idLower.contains("tablet") || nameLower.contains("tablet") ||
        idLower.contains("macbook") || nameLower.contains("macbook")) 
    {
        return true;
    }

    return false;
}

QString InternetAccessModel::determineType(const QString& entityId, const QString& name) const
{
    QString idLower = entityId.toLower();
    if (idLower.startsWith(QStringLiteral("switch."))) {
        idLower = idLower.sliced(7);
    }
    QString nameLower = name.toLower();

    if (idLower.contains("tv") || nameLower.contains("tv") || nameLower.contains("television")) {
        return QStringLiteral("television");
    }
    if (idLower.contains("phone") || nameLower.contains("phone") || idLower.contains("mobile") || nameLower.contains("mobile") ||
        idLower.contains("ipad") || nameLower.contains("ipad") || idLower.contains("tablet") || nameLower.contains("tablet")) 
    {
        return QStringLiteral("smartphone");
    }
    if (idLower.contains("playstation") || nameLower.contains("playstation") ||
        idLower.contains("xbox") || nameLower.contains("xbox") ||
        idLower.contains("nintendo") || nameLower.contains("nintendo") ||
        idLower.contains("switch") || nameLower.contains("switch")) 
    {
        return QStringLiteral("console");
    }
    return QStringLiteral("generic");
}

QString InternetAccessModel::extractDeviceKey(const QString& entityId) const
{
    QString id = entityId.toLower();
    if (id.startsWith(QStringLiteral("switch."))) {
        id = id.sliced(7);
    } else if (id.startsWith(QStringLiteral("device_tracker."))) {
        id = id.sliced(15);
    }

    // Eliminar sufijos comunes de switches de internet
    if (id.endsWith(QStringLiteral("_acceso_a_internet"))) {
        id = id.first(id.length() - 18); // _acceso_a_internet tiene 18 caracteres
    } else if (id.endsWith(QStringLiteral("_internet_access"))) {
        id = id.first(id.length() - 16);
    } else if (id.endsWith(QStringLiteral("_internet"))) {
        id = id.first(id.length() - 9);
    }
    return id;
}

QVector<BlockedDevice> InternetAccessModel::getFilteredDevices() const
{
    QVector<BlockedDevice> filtered;
    for (const auto& device : m_devices) {
        QString key = extractDeviceKey(device.entityId);
        if (m_onlineDevices.contains(key)) {
            filtered.append(device);
        }
    }
    return filtered;
}

void InternetAccessModel::clear()
{
    beginResetModel();
    m_devices.clear();
    m_onlineDevices.clear();
    endResetModel();
    emit activeCountChanged();
}
