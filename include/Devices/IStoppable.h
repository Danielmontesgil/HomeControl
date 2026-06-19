#pragma once

class IStoppable
{
public:
    virtual ~IStoppable() = default;
	virtual bool isMoving() const = 0;
	virtual void stop() = 0;
};
