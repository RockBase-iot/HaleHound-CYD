#pragma once

#include "drivers/IPeripheralDriver.h"
#include "services/SpiBusService.h"

class Cc1101Driver final : public IPeripheralDriver {
public:
    explicit Cc1101Driver(SpiBusService* spi = nullptr) : spi_(spi) {}

    PeripheralId id() const override { return PeripheralId::Cc1101; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;

    uint8_t lastVersion = 0;

private:
    SpiBusService* spi_;
};
