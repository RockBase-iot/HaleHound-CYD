#pragma once

#include <cstdint>

enum class SpiDeviceId : uint8_t {
    Display,
    Touch,
    Sd,
    Cc1101,
    Nrf24,
    Pn532,
};

class ISpiBackend {
public:
    virtual ~ISpiBackend() = default;
    virtual bool lock(uint32_t timeoutMs) = 0;
    virtual void unlock() = 0;
    virtual void deselectAll() = 0;
    virtual bool beginTransaction(SpiDeviceId device) = 0;
    virtual bool select(SpiDeviceId device) = 0;
    virtual uint8_t transfer(uint8_t value) = 0;
    virtual void deselect(SpiDeviceId device) = 0;
    virtual void endTransaction() = 0;
};
