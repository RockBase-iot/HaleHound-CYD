#pragma once

#include "drivers/IPeripheralDriver.h"

class Pn532I2cDriver final : public IPeripheralDriver {
public:
    PeripheralId id() const override { return PeripheralId::Pn532; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;
};
