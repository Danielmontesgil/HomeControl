#include "DeviceModel.h"
#include "HomeDeviceBase.h"
#include "IValuable.h"
#include "ISwitchable.h"
#include "IStoppable.h"

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
        {IsOnRole, "isOn"}
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
