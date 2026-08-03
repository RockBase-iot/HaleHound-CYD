#pragma once

#include "services/ISpiBackend.h"

class ArduinoSpiBackend final : public ISpiBackend {
public:
    bool initialize();
    bool lock(uint32_t timeoutMs) override;
    void unlock() override;
    void deselectAll() override;
    bool beginTransaction(SpiDeviceId device) override;
    bool select(SpiDeviceId device) override;
    uint8_t transfer(uint8_t value) override;
    void deselect(SpiDeviceId device) override;
    void endTransaction() override;
};
