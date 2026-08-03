#pragma once

#include <cstdint>
#include <vector>

#include "drivers/IPeripheralDriver.h"

struct IrFrame;

class IrTxDriver final : public IPeripheralDriver {
public:
    PeripheralId id() const override { return PeripheralId::IrTx; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;
    bool send(const IrFrame& frame, bool loopbackConfirmed) const;
    void setLoopbackConfirmed(bool confirmed) { confirmed_ = confirmed; }
    static constexpr uint8_t mutexGroup() { return 2; }

private:
    bool confirmed_ = false;
};
