#pragma once

#include <cstdint>

#include "services/IResourceMutex.h"

class ArduinoResourceMutex final : public IResourceMutex {
public:
    ArduinoResourceMutex();
    ~ArduinoResourceMutex() override;

    bool lock(uint32_t timeoutMs) override;
    void unlock() override;

private:
#ifdef ARDUINO
    void* handle_ = nullptr;
#else
    bool locked_ = false;
#endif
};
