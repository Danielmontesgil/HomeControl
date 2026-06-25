#pragma once
#include <QString>

/**
 * @brief Interface for devices supporting color (RGB) control.
 */
class IColorable
{
public:
    virtual ~IColorable() = default;

    /**
     * @brief Gets the current color in hex format (e.g. "#FF0000" for red).
     */
    virtual QString getColor() const = 0;

    /**
     * @brief Sets the device color using a hex string.
     */
    virtual void setColor(const QString& hexColor) = 0;
};
