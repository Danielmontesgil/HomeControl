#pragma once
#include <memory>
#include <string>
#include <functional>
#include "HomeDevice.h"

// Interfaz para la Inversión de Dependencias
class IDeviceFactory
{
public:
    virtual std::unique_ptr<HomeDevice> create(const std::string& type, const std::string& id) = 0;
    
    // Método para registrar nuevos tipos dinámicamente sin modificar la clase
    virtual void registerType(const std::string& type, std::function<std::unique_ptr<HomeDevice>(const std::string&)> creator) = 0;
    
    virtual ~IDeviceFactory() = default;
};
