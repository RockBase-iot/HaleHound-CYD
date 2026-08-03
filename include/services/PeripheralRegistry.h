#pragma once

#include <cstddef>
#include <cstdint>

#include "board/BoardProfile.h"
#include "drivers/IPeripheralDriver.h"

struct PeripheralDiagnostic {
    PeripheralStatus status;
    const char* detail;
};

class PeripheralRegistry {
public:
    using Clock = uint32_t (*)();

    explicit PeripheralRegistry(BoardId board,
                                 Clock clock = nullptr,
                                 uint32_t cacheTtlMs = 5000U);

    bool registerDriver(IPeripheralDriver& driver);
    PeripheralDiagnostic ensureAvailable(PeripheralId peripheral,
                                          uint32_t timeoutMs = 50U);
    PeripheralStatus status(PeripheralId peripheral) const;
    PeripheralDiagnostic diagnostic(PeripheralId peripheral) const;
    void invalidate(PeripheralId peripheral);

private:
    static constexpr size_t kPeripheralCount =
        static_cast<size_t>(PeripheralId::Sd) + 1U;

    struct Entry {
        IPeripheralDriver* driver = nullptr;
        PeripheralStatus status = PeripheralStatus::Unverified;
        const char* detail = "not probed";
        uint32_t checkedAt = 0;
    };

    static size_t indexOf(PeripheralId peripheral);
    uint32_t now() const;

    BoardId board_;
    Clock clock_;
    uint32_t cacheTtlMs_;
    Entry entries_[kPeripheralCount]{};
};
