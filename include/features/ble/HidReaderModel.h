#pragma once

// HID reader tooling.
//
// Upstream v4.0 added a BLE module for locating and stress-testing HID Signo
// and iCLASS SE readers: detected by advertised service UUID with a passive
// scan plus a connect diagnostic, the reader's reply captured over
// notifications, captures logged to the card, and an honest end-of-transaction
// status. The Artemis core sweep is deliberately split across connects, two
// commands at a time, so it completes instead of timing out.
//
// The sweep scheduler and the transaction reporting are the testable parts;
// this module contains both and no radio code.

#include <cstdint>

class IFeature;

constexpr uint8_t kHidSweepCommandsPerConnect = 2;

enum class HidReaderKind : uint8_t {
    Unknown,
    Signo,
    IclassSe,
};

enum class HidTransactionState : uint8_t {
    Idle,
    Scanning,
    Connecting,
    Capturing,
    Completed,
    Failed,
};

struct HidTransactionReport {
    HidTransactionState state = HidTransactionState::Idle;
    // True only when the reader actually produced a notification this pass.
    bool replyCaptured = false;
    // A pass that produced no reply never claims to have completed.
    bool completed = false;
    const char* reason = "not started";
};

// Round-robin command scheduler. Each connect gets at most
// kHidSweepCommandsPerConnect commands, which is what keeps a long core sweep
// from blowing the connection timeout.
class HidSweepScheduler {
public:
    void begin(uint8_t totalCommands);
    void reset();

    // Fills ``out`` with up to ``capacity`` commands for the current connect and
    // returns how many were written. Returns 0 when the sweep is exhausted.
    uint8_t nextBatch(uint8_t* out, uint8_t capacity);

    uint8_t totalCommands() const { return totalCommands_; }
    uint8_t issuedCommands() const { return issuedCommands_; }
    uint8_t connectsUsed() const { return connectsUsed_; }
    bool exhausted() const { return issuedCommands_ >= totalCommands_; }

private:
    uint8_t totalCommands_ = 0;
    uint8_t issuedCommands_ = 0;
    uint8_t connectsUsed_ = 0;
};

class HidReaderModel {
public:
    // Service UUID based identification. Returns Unknown rather than guessing
    // when the advertisement does not carry a known reader service.
    static HidReaderKind classify(uint16_t advertisedServiceUuid);
    static const char* kindLabel(HidReaderKind kind);
    static const char* stateLabel(HidTransactionState state);

    // Builds the end-of-transaction report. ``replyCaptured`` is the only thing
    // that can promote the pass to completed.
    static HidTransactionReport report(bool requested, bool connectSucceeded, bool replyCaptured);
};

IFeature* createHidReaderFeature();
