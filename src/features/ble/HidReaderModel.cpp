#include "features/ble/HidReaderModel.h"

#include "core/FeatureRunner.h"

void HidSweepScheduler::begin(uint8_t totalCommands) {
    totalCommands_ = totalCommands;
    issuedCommands_ = 0;
    connectsUsed_ = 0;
}

void HidSweepScheduler::reset() {
    totalCommands_ = 0;
    issuedCommands_ = 0;
    connectsUsed_ = 0;
}

uint8_t HidSweepScheduler::nextBatch(uint8_t* out, uint8_t capacity) {
    if (out == NULL || capacity == 0 || exhausted()) {
        return 0;
    }
    uint8_t limit = capacity < kHidSweepCommandsPerConnect ? capacity : kHidSweepCommandsPerConnect;
    const uint8_t remaining = static_cast<uint8_t>(totalCommands_ - issuedCommands_);
    if (limit > remaining) {
        limit = remaining;
    }
    for (uint8_t index = 0; index < limit; ++index) {
        out[index] = issuedCommands_;
        ++issuedCommands_;
    }
    ++connectsUsed_;
    return limit;
}

HidReaderKind HidReaderModel::classify(uint16_t advertisedServiceUuid) {
    switch (advertisedServiceUuid) {
        case 0xFD40:
            return HidReaderKind::Signo;
        case 0xFFF0:
            return HidReaderKind::IclassSe;
        default:
            return HidReaderKind::Unknown;
    }
}

const char* HidReaderModel::kindLabel(HidReaderKind kind) {
    switch (kind) {
        case HidReaderKind::Signo:
            return "HID Signo";
        case HidReaderKind::IclassSe:
            return "iCLASS SE";
        case HidReaderKind::Unknown:
        default:
            return "Unknown reader";
    }
}

const char* HidReaderModel::stateLabel(HidTransactionState state) {
    switch (state) {
        case HidTransactionState::Scanning:
            return "SCANNING";
        case HidTransactionState::Connecting:
            return "CONNECTING";
        case HidTransactionState::Capturing:
            return "CAPTURING";
        case HidTransactionState::Completed:
            return "COMPLETED";
        case HidTransactionState::Failed:
            return "FAILED";
        case HidTransactionState::Idle:
        default:
            return "IDLE";
    }
}

HidTransactionReport HidReaderModel::report(bool requested,
                                            bool connectSucceeded,
                                            bool replyCaptured) {
    HidTransactionReport report;
    if (!requested) {
        report.reason = "not requested";
        return report;
    }
    if (!connectSucceeded) {
        report.state = HidTransactionState::Failed;
        report.reason = "connect failed";
        return report;
    }
    report.replyCaptured = replyCaptured;
    if (!replyCaptured) {
        report.state = HidTransactionState::Failed;
        report.reason = "reader produced no notification";
        return report;
    }
    report.state = HidTransactionState::Completed;
    report.completed = true;
    report.reason = "reply captured";
    return report;
}

namespace {

class HidReaderFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        scheduler_.begin(0);
        return {FeatureResultCode::Completed, "HID reader scan ready"};
    }

    void tick() override {}

    void requestStop(StopReason) override { scheduler_.reset(); }

    FeatureResult end() override {
        scheduler_.reset();
        return {FeatureResultCode::Completed, "HID reader closed"};
    }

private:
    HidSweepScheduler scheduler_;
};

}  // namespace

IFeature* createHidReaderFeature() {
    return new HidReaderFeature();
}
