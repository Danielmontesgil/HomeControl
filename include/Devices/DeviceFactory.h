#pragma once
#include "IDeviceFactory.h"
#include <unordered_map>

class DeviceFactory : public IDeviceFactory
{
public:
    DeviceFactory() = default;
    ~DeviceFactory() override = default;

    std::unique_ptr<HomeDevice> create(const std::string& type, const std::string& id) override;
    void registerType(const std::string& type, std::function<std::unique_ptr<HomeDevice>(const std::string&)> creator) override;

private:
    // El "Registro" (Registry): Un mapa que asocia un string (tipo) con una función creadora
    std::unordered_map<std::string, std::function<std::unique_ptr<HomeDevice>(const std::string&)>> m_creators;
};
