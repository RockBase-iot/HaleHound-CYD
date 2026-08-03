#pragma once

#include <cstdint>

class IResourceMutex {
public:
    virtual ~IResourceMutex() = default;
    virtual bool lock(uint32_t timeoutMs) = 0;
    virtual void unlock() = 0;
};
