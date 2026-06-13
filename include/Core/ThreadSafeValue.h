#pragma once
#include <mutex>

template<typename T>
class ThreadSafeValue
{
private:
    T value;
    mutable std::mutex mtx;
    
public:
    ThreadSafeValue(T initial) : value(initial) {}
    
    void set(T newValue)
    {
        std::lock_guard<std::mutex> lock(mtx);
        value = newValue;
    }
    
    T get() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }
    
};
