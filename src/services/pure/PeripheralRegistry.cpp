#include "services/PeripheralRegistry.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

PeripheralRegistry::PeripheralRegistry(BoardId board,
                                       Clock clock,
                                       uint32_t cacheTtlMs)
    : board_(board), clock_(clock), cacheTtlMs_(cacheTtlMs) {}

size_t PeripheralRegistry::indexOf(PeripheralId peripheral) {
    const size_t index = static_cast<size_t>(peripheral);
    return index < kPeripheralCount ? index : kPeripheralCount;
}

uint32_t PeripheralRegistry::now() const {
    if (clock_ != nullptr) {
        return clock_();
    }
#ifdef ARDUINO
    return millis();
#else
    return 0;
#endif
}

bool PeripheralRegistry::registerDriver(IPeripheralDriver& driver) {
    const size_t index = indexOf(driver.id());
    if (index >= kPeripheralCount) {
        return false;
    }
    entries_[index].driver = &driver;
    entries_[index].status = PeripheralStatus::Unverified;
    entries_[index].detail = "not probed";
    entries_[index].checkedAt = 0;
    return true;
}

PeripheralDiagnostic PeripheralRegistry::ensureAvailable(PeripheralId peripheral,
                                                         uint32_t timeoutMs) {
    const size_t index = indexOf(peripheral);
    if (index >= kPeripheralCount) {
        return {PeripheralStatus::Unsupported, "unknown peripheral"};
    }

    Entry& entry = entries_[index];
    const ExpansionRoute& route = findRoute(board_, peripheral);
    if (route.transport == Transport::None) {
        entry.status = PeripheralStatus::Unsupported;
        entry.detail = route.wiringHint;
        entry.checkedAt = now();
        return {entry.status, entry.detail};
    }
    if (entry.driver == nullptr) {
        entry.status = PeripheralStatus::Unsupported;
        entry.detail = "driver unavailable";
        entry.checkedAt = now();
        return {entry.status, entry.detail};
    }

    const uint32_t current = now();
    const bool cached = entry.status != PeripheralStatus::Unverified &&
                        entry.status != PeripheralStatus::Busy &&
                        (cacheTtlMs_ == 0U || current - entry.checkedAt < cacheTtlMs_);
    if (cached) {
        return {entry.status, entry.detail};
    }

    const ProbeResult result = entry.driver->probe(route, timeoutMs);
    entry.status = result.status;
    entry.detail = result.detail != nullptr ? result.detail : "probe complete";
    entry.checkedAt = current;
    return {entry.status, entry.detail};
}

PeripheralStatus PeripheralRegistry::status(PeripheralId peripheral) const {
    const size_t index = indexOf(peripheral);
    return index < kPeripheralCount ? entries_[index].status
                                    : PeripheralStatus::Unsupported;
}

PeripheralDiagnostic PeripheralRegistry::diagnostic(PeripheralId peripheral) const {
    const size_t index = indexOf(peripheral);
    if (index >= kPeripheralCount) {
        return {PeripheralStatus::Unsupported, "unknown peripheral"};
    }
    return {entries_[index].status, entries_[index].detail};
}

void PeripheralRegistry::invalidate(PeripheralId peripheral) {
    const size_t index = indexOf(peripheral);
    if (index < kPeripheralCount) {
        entries_[index].status = PeripheralStatus::Unverified;
        entries_[index].detail = "not probed";
        entries_[index].checkedAt = 0;
    }
}
