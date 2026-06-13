#include "DeviceFactory.h"

void DeviceFactory::registerType(const std::string& type, std::function<std::unique_ptr<HomeDevice>(const std::string&)> creator)
{
    m_creators[type] = std::move(creator);
}

std::unique_ptr<HomeDevice> DeviceFactory::create(const std::string& type, const std::string& id)
{
    // Buscamos el tipo en nuestro registro O(1)
    auto it = m_creators.find(type);
    
    if (it != m_creators.end())
    {
        // Si existe, ejecutamos su función creadora y le pasamos el ID
        return it->second(id);
    }
    
    // Si el tipo no está registrado, devolvemos un puntero nulo de forma segura
    return nullptr;
}
