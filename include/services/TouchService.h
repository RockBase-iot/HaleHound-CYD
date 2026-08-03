#pragma once

#include "services/SpiBusService.h"

class TouchService {
public:
    explicit TouchService(SpiBusService& spi) : spi_(spi) {}

    bool sample(SpiTransactionCallback callback, uint32_t timeoutMs = 0);

private:
    SpiBusService& spi_;
};
