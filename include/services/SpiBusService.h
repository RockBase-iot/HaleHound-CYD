#pragma once

#include <cstdint>

#include "services/ISpiBackend.h"
#include "services/ResourceBroker.h"

enum class SpiResultCode : uint8_t {
    Completed,
    Busy,
    Timeout,
    Fault,
    Invalid,
};

struct SpiResult {
    SpiResultCode code;
    const char* detail;
};

using SpiTransactionCallback = bool (*)(ISpiBackend& backend);

class SpiBusService {
public:
    SpiBusService(ResourceBroker& broker, ISpiBackend& backend)
        : broker_(broker), backend_(backend) {}

    SpiResult transact(SpiDeviceId device,
                      SpiTransactionCallback callback,
                      uint32_t timeoutMs);

    SpiResult beginLegacySession(SpiDeviceId device,
                                 uint32_t timeoutMs = 0,
                                 bool manageTransaction = true);
    SpiResult endLegacySession();
    bool hasLegacySession() const { return legacyActive_; }

    static ResourceId resourceFor(SpiDeviceId) {
        return ResourceId::SharedSpi;
    }

private:
    static OwnerId ownerFor(SpiDeviceId device);

    ResourceBroker& broker_;
    ISpiBackend& backend_;
    ResourceLease legacyLease_;
    SpiDeviceId legacyDevice_ = SpiDeviceId::Display;
    bool legacyActive_ = false;
    bool legacyLocked_ = false;
    bool legacyTransactionStarted_ = false;
    bool legacySelected_ = false;
};
