#pragma once
#include "IDeviceFactory.h"
#include <unordered_map>

class HomeDeviceBase;

class DeviceFactory : public IDeviceFactory
{
public:
    DeviceFactory();
    ~DeviceFactory() override;

    std::unique_ptr<HomeDeviceBase> create(const std::string& type, const std::string& id, const std::string& topic) override;
    void registerType(const std::string& type, std::function<std::unique_ptr<HomeDeviceBase>(const std::string&, const std::string&)> creator) override;

private:
    std::unordered_map<std::string, std::function<std::unique_ptr<HomeDeviceBase>(const std::string&, const std::string&)>> m_creators;
};
