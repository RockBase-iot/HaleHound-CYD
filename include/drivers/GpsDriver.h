#pragma once

#include <cstddef>

#include "drivers/IPeripheralDriver.h"

class IGpsProbeSource {
public:
    virtual ~IGpsProbeSource() = default;
    virtual size_t available() = 0;
    virtual int read() = 0;
};

class GpsDriver final : public IPeripheralDriver {
public:
    explicit GpsDriver(IGpsProbeSource* source = nullptr) : source_(source) {}

    PeripheralId id() const override { return PeripheralId::Gps; }
    ProbeResult probe(const ExpansionRoute& route, uint32_t timeoutMs) override;

private:
    IGpsProbeSource* source_;
};
