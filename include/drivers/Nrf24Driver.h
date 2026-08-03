#pragma once

#include "drivers/IPeripheralDriver.h"
#include "services/SpiBusService.h"

class Nrf24Driver final : public IPeripheralDriver {
public:
    explicit Nrf24Driver(SpiBusService* spi = nullptr) : spi_(spi) {}

    PeripheralId id() const override { return PeripheralId::Nrf24; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;

    uint8_t config = 0;
    uint8_t status = 0;

private:
    SpiBusService* spi_;
};
