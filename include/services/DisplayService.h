#pragma once

#include "services/SpiBusService.h"

class DisplayService {
public:
    explicit DisplayService(SpiBusService& spi) : spi_(spi) {}

    bool renderRegion(SpiTransactionCallback callback, uint32_t timeoutMs = 0);

private:
    SpiBusService& spi_;
};
