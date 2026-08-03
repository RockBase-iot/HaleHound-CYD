#include "core/BootHealthService.h"

#ifdef ARDUINO
#include <Preferences.h>

namespace {
class PreferencesBootStore final : public IBootHealthStore {
public:
    bool load(BootHealthRecord& output) override {
        preferences_.begin("boot-health", true);
        const size_t length = preferences_.getBytesLength("record");
        const bool loaded = length == sizeof(output) &&
                            preferences_.getBytes("record", &output, sizeof(output)) == sizeof(output);
        preferences_.end();
        return loaded;
    }
    bool save(const BootHealthRecord& input) override {
        preferences_.begin("boot-health", false);
        const size_t written = preferences_.putBytes("record", &input, sizeof(input));
        preferences_.end();
        return written == sizeof(input);
    }

private:
    Preferences preferences_;
};

PreferencesBootStore store;
BootHealthService service(store);
}

BootHealthService& globalBootHealthService() { return service; }
#else
namespace {
class UnavailableBootStore final : public IBootHealthStore {
public:
    bool load(BootHealthRecord&) override { return false; }
    bool save(const BootHealthRecord&) override { return false; }
};

UnavailableBootStore store;
BootHealthService service(store);
}

BootHealthService& globalBootHealthService() { return service; }
#endif

bool BootHealthService::begin() {
    BootHealthRecord loaded{};
    if (!store_.load(loaded) || loaded.magic != kMagic) {
        record_ = BootHealthRecord{kMagic, 0U, 0U, 0U};
        state_ = BootHealthState::Initial;
        return store_.save(record_);
    }
    record_ = loaded;
    if (record_.pending != 0U) {
        if (record_.failures < 0xFFU) ++record_.failures;
        if (record_.failures >= failureThreshold_) record_.safeMode = 1U;
    }
    state_ = record_.safeMode != 0U ? BootHealthState::SafeMode
                                    : (record_.pending != 0U ? BootHealthState::Pending
                                                              : BootHealthState::Initial);
    return store_.save(record_);
}

bool BootHealthService::markPending() {
    record_.magic = kMagic;
    record_.pending = 1U;
    state_ = record_.safeMode != 0U ? BootHealthState::SafeMode : BootHealthState::Pending;
    return store_.save(record_);
}

bool BootHealthService::markHealthy() {
    record_.magic = kMagic;
    record_.pending = 0U;
    state_ = record_.safeMode != 0U ? BootHealthState::SafeMode : BootHealthState::Healthy;
    return store_.save(record_);
}

bool BootHealthService::clearSafeMode() {
    record_.magic = kMagic;
    record_.safeMode = 0U;
    record_.pending = 0U;
    record_.failures = 0U;
    state_ = BootHealthState::Healthy;
    return store_.save(record_);
}
