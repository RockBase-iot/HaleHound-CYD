#pragma once

#include <cstdint>

#include "board/ExpansionRoute.h"

enum class PeripheralStatus : uint8_t {
    Unverified,
    Available,
    NotFound,
    Busy,
    Fault,
    Unsupported,
};

using PeripheralState = PeripheralStatus;

struct ProbeResult {
    PeripheralStatus status;
    const char* detail;
};

class IPeripheralDriver {
public:
    virtual ~IPeripheralDriver() = default;
    virtual PeripheralId id() const = 0;
    virtual ProbeResult probe(const ExpansionRoute& route,
                              uint32_t timeoutMs) = 0;
};
