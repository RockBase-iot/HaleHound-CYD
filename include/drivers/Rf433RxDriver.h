#pragma once

#include <cstdint>

#include "drivers/IPeripheralDriver.h"

struct Rf433Signal;

class Rf433RxDriver final : public IPeripheralDriver {
public:
    PeripheralId id() const override { return PeripheralId::Rf433Rx; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;
    bool receive(Rf433Signal& signal) const;
    void setLoopbackConfirmed(bool confirmed) { confirmed_ = confirmed; }
    static constexpr uint8_t mutexGroup() { return 2; }

private:
    bool confirmed_ = false;
};
