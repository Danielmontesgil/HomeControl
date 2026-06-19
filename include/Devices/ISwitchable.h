#pragma once

class ISwitchable {
public:
    virtual ~ISwitchable() = default;
    virtual bool isOn() const = 0;
};
