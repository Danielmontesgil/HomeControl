#include "DeviceFactory.h"
#include "HomeDeviceBase.h"

DeviceFactory::DeviceFactory() = default;
DeviceFactory::~DeviceFactory() = default;

void DeviceFactory::registerType(const std::string& type, std::function<std::unique_ptr<HomeDeviceBase>(const std::string&, const std::string&)> creator)
{
    m_creators[type] = std::move(creator);
}

std::unique_ptr<HomeDeviceBase> DeviceFactory::create(const std::string& type, const std::string& id, const std::string& topic)
{
    // Buscamos el tipo en nuestro registro O(1)
    auto it = m_creators.find(type);
    
    if (it != m_creators.end())
    {
        // Si existe, ejecutamos su función creadora y le pasamos el ID
        return it->second(id, topic);
    }
    
    // Si el tipo no está registrado, devolvemos un puntero nulo de forma segura
    return nullptr;
}
