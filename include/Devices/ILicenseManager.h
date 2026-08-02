#pragma once
#include <string>

class ILicenseManager {
public:
    virtual ~ILicenseManager() = default;
    
    // Devuelve true si la característica identificada por featureId está activa/licenciada
    virtual bool isFeatureLicensed(const std::string& featureId) const = 0;
    
    // Intenta activar una característica mediante un código de licencia local.
    // Retorna true si es válido y se activa con éxito.
    virtual bool activateFeature(const std::string& featureId, const std::string& licenseKey) = 0;
};
