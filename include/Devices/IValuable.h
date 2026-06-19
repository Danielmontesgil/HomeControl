#pragma once

class IValuable {
public:
    virtual ~IValuable() = default;
    virtual float getValue() const = 0;
};
