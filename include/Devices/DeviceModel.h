#pragma once
#include <QAbstractListModel>
#include <vector>
#include <memory>
#include <unordered_map>

class HomeDeviceBase;

class DeviceModel : public QAbstractListModel
{
	Q_OBJECT

public:
	enum DeviceRoles {
		IdRole = Qt::UserRole + 1,
		TopicRole,
		ValueRole,
		TypeRole,
		SupportsStopRole,
		IsMovingRole,
		IsOnRole,
		ColorRole,
		SupportsColorRole
	};

	explicit DeviceModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	void addDevice(std::unique_ptr<HomeDeviceBase> device);
	HomeDeviceBase* findByTopic(const QString& topic) const;

private:
	std::vector<std::unique_ptr<HomeDeviceBase>> m_devices;
	std::unordered_map<HomeDeviceBase*, int> m_devicetoRow;
};
