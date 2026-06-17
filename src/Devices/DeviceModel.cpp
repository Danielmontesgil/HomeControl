#include "DeviceModel.h"
#include "HomeDeviceBase.h"

DeviceModel::DeviceModel(QObject* parent) : QAbstractListModel (parent){}

QHash<int, QByteArray> DeviceModel::roleNames() const
{
    return {{IdRole, "deviceId"}, {TopicRole, "topic"}, {ValueRole, "deviceValue"}, {TypeRole, "deviceType"}};
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
            return device->getValue();
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
    
    connect(ptr, &HomeDeviceBase::valueChanged, this, [this, ptr]()
    {
        int row = m_devicetoRow[ptr];
        
        QModelIndex idx = index(row);
        
        emit dataChanged(idx, idx, {ValueRole});
    });
}
