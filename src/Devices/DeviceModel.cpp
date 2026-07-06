#include "DeviceModel.h"
#include "HomeDeviceBase.h"
#include "IValuable.h"
#include "ISwitchable.h"
#include "IStoppable.h"
#include "IColorable.h"
#include "VacuumDevice.h"

DeviceModel::DeviceModel(QObject* parent) : QAbstractListModel (parent){}

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
        {FanSpeedRole, "fanSpeed"}
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
            if (auto* valuable = dynamic_cast<IValuable*>(device.get()))
            {
                return valuable->getValue();
            }
            return QVariant();
        case IsOnRole:
            if (auto* switchable = dynamic_cast<ISwitchable*>(device.get()))
            {
                return switchable->isOn();
            }
            return false;
        case SupportsStopRole:
            return dynamic_cast<IStoppable*>(device.get()) != nullptr;
        case IsMovingRole:
            if (auto* stoppable = dynamic_cast<IStoppable*>(device.get()))
            {
                return stoppable->isMoving();
            }
            return false;
        case TypeRole:
            return static_cast<int>(device->getType());
        case ColorRole:
            if (auto* colorable = dynamic_cast<IColorable*>(device.get()))
            {
                return colorable->getColor();
            }
            return QVariant();
        case SupportsColorRole:
            return dynamic_cast<IColorable*>(device.get()) != nullptr;
        case VacuumStateRole:
            if (auto* vacuum = dynamic_cast<VacuumDevice*>(device.get()))
            {
                return vacuum->getVacuumState();
            }
            return QVariant();
        case BatteryLevelRole:
            if (auto* vacuum = dynamic_cast<VacuumDevice*>(device.get()))
            {
                return vacuum->getBatteryLevel();
            }
            return QVariant();
        case FanSpeedRole:
            if (auto* vacuum = dynamic_cast<VacuumDevice*>(device.get()))
            {
                return vacuum->getFanSpeed();
            }
            return QVariant();
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
