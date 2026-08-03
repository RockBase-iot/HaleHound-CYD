#pragma once

#include <cstdint>

#include "drivers/IPeripheralDriver.h"

struct IrFrame;

class IrRxDriver final : public IPeripheralDriver {
public:
    PeripheralId id() const override { return PeripheralId::IrRx; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;
    bool capture(IrFrame& frame) const;
    void setLoopbackConfirmed(bool confirmed) { confirmed_ = confirmed; }
    static constexpr uint8_t mutexGroup() { return 2; }

private:
    bool confirmed_ = false;
};
