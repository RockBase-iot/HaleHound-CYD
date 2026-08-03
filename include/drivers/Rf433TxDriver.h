#pragma once

#include <cstdint>

#include "drivers/IPeripheralDriver.h"

struct Rf433Signal;

class Rf433TxDriver final : public IPeripheralDriver {
public:
    PeripheralId id() const override { return PeripheralId::Rf433Tx; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;
    bool send(const Rf433Signal& signal, bool loopbackConfirmed) const;
    void setLoopbackConfirmed(bool confirmed) { confirmed_ = confirmed; }
    static constexpr uint8_t mutexGroup() { return 2; }

private:
    bool confirmed_ = false;
};
