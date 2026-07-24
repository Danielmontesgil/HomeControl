#include "DeviceModel.h"
#include "HomeDeviceBase.h"
#include "SwitchableComponent.h"
#include "DimmableComponent.h"
#include "ColorableComponent.h"
#include "StoppableComponent.h"
#include "StatusComponent.h"

DeviceModel::DeviceModel(QObject* parent) : QAbstractListModel (parent){}
DeviceModel::~DeviceModel() = default;

QHash<int, QByteArray> DeviceModel::roleNames() const
{
    return {
        {IdRole, "deviceId"}, 
        {TopicRole, "topic"}, 
        {ValueRole, "deviceValue"}, 
        {TypeRole, "deviceType"}, 
        {SupportsStopRole, "supportsStop"}, 
        {IsMovingRole, "isMoving"},
        {IsOnRole, "isOn"},
        {ColorRole, "deviceColor"},
        {SupportsColorRole, "supportsColor"},
        {VacuumStateRole, "vacuumState"},
        {BatteryLevelRole, "batteryLevel"},
        {FanSpeedRole, "fanSpeed"},
        {CapabilitiesRole, "capabilities"},
        {AvailableRole, "available"}
    };
}

int DeviceModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_devices.size());
}

QVariant DeviceModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_devices.size())
    {
        return QVariant();
    }
    
    const auto& device = m_devices[index.row()];
    
    switch (role)
    {
        case IdRole:
            return device->getQStringId();
        case TopicRole:
            return device->getQStringTopic();
        case ValueRole:
            if (auto* dimmable = device->getComponent("dimmable"))
            {
                return static_cast<DimmableComponent*>(dimmable)->getLevel();
            }
            return QVariant();
        case IsOnRole:
            if (auto* switchable = device->getComponent("switchable"))
            {
                return static_cast<SwitchableComponent*>(switchable)->isOn();
            }
            return false;
        case SupportsStopRole:
            return device->hasComponent("stoppable");
        case IsMovingRole:
            if (auto* stoppable = device->getComponent("stoppable"))
            {
                return static_cast<StoppableComponent*>(stoppable)->isMoving();
            }
            return false;
        case TypeRole:
            return static_cast<int>(device->getType());
        case ColorRole:
            if (auto* colorable = device->getComponent("colorable"))
            {
                return static_cast<ColorableComponent*>(colorable)->getColor();
            }
            return QVariant();
        case SupportsColorRole:
            return device->hasComponent("colorable");
        case VacuumStateRole:
            if (auto* status = device->getComponent("status"))
            {
                return static_cast<StatusComponent*>(status)->getStatusState();
            }
            return QVariant();
        case BatteryLevelRole:
            if (auto* status = device->getComponent("status"))
            {
                return static_cast<StatusComponent*>(status)->getBatteryLevel();
            }
            return QVariant();
        case FanSpeedRole:
            if (auto* status = device->getComponent("status"))
            {
                return static_cast<StatusComponent*>(status)->getFanSpeed();
            }
            return QVariant();
        case CapabilitiesRole:
            return QVariant::fromValue(device->getComponentNames());
        case AvailableRole:
            return device->isAvailable();
    }
    
    return QVariant();
}

void DeviceModel::addDevice(std::unique_ptr<HomeDeviceBase> device)
{
    int newRow = static_cast<int>(m_devices.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    
    m_devices.push_back(std::move(device));
    m_devicetoRow[m_devices.back().get()] = newRow;
    
    endInsertRows();
    
    HomeDeviceBase* ptr = m_devices.back().get();
    
    connect(ptr, &HomeDeviceBase::updated, this, [this, ptr]()
    {
        int row = m_devicetoRow[ptr];
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx); 
    });
}

HomeDeviceBase* DeviceModel::findByTopic(const QString& topic) const
{
    for (const auto& device : m_devices)
    {
        if (device->getQStringTopic() == topic) return device.get();
    }
    return nullptr;
}
