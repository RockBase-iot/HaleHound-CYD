#pragma once

#include <cstdint>

enum class BootHealthState : uint8_t {
    Initial,
    Pending,
    Healthy,
    SafeMode,
};

struct BootHealthRecord {
    constexpr BootHealthRecord(uint16_t recordMagic = 0U,
                               uint8_t recordPending = 0U,
                               uint8_t recordFailures = 0U,
                               uint8_t recordSafeMode = 0U)
        : magic(recordMagic), pending(recordPending), failures(recordFailures),
          safeMode(recordSafeMode) {}

    uint16_t magic = 0U;
    uint8_t pending = 0U;
    uint8_t failures = 0U;
    uint8_t safeMode = 0U;
};

class IBootHealthStore {
public:
    virtual ~IBootHealthStore() = default;
    virtual bool load(BootHealthRecord& record) = 0;
    virtual bool save(const BootHealthRecord& record) = 0;
};

class BootHealthService {
public:
    explicit BootHealthService(IBootHealthStore& store, uint8_t failureThreshold = 3U)
        : store_(store), failureThreshold_(failureThreshold == 0U ? 1U : failureThreshold) {}

    bool begin();
    bool markPending();
    bool markHealthy();
    bool clearSafeMode();
    bool safeMode() const { return state_ == BootHealthState::SafeMode; }
    BootHealthState state() const { return state_; }
    uint8_t failures() const { return record_.failures; }

private:
    static constexpr uint16_t kMagic = 0xB007U;
    IBootHealthStore& store_;
    uint8_t failureThreshold_;
    BootHealthRecord record_{};
    BootHealthState state_ = BootHealthState::Initial;
};

BootHealthService& globalBootHealthService();
