#pragma once
#include <memory>
#include <string>
#include <functional>

class HomeDeviceBase;

class IDeviceFactory
{
public:
    virtual std::unique_ptr<HomeDeviceBase> create(const std::string& type, const std::string& id, const std::string& topic) = 0;
    
    virtual void registerType(const std::string& type, std::function<std::unique_ptr<HomeDeviceBase>(const std::string&, const std::string&)> creator) = 0;
    
    virtual ~IDeviceFactory() = default;
};
