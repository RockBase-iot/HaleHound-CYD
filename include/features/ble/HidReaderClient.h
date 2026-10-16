#pragma once

// HID reader tooling over a real BLE controller.
//
// HidReaderModel has the transaction report and the sweep scheduler; this is the
// part that drives a radio through them. The interesting question here is not
// "does it connect" but "what is it allowed to claim".
//
// It is not allowed to claim it identified a reader. The service UUIDs this
// build matches on -- 0xFD40 for the Signo, 0xFFF0 for iCLASS SE -- have no
// authoritative source, and 0xFFF0 is the Triones / ELK-BLEDOM vendor service in
// the SIG's own list, which means the number is doing double duty across
// contexts (REMAINING_WORK X3). So a candidate carries the grade of its
// identification: a match on 0xFFF0 is reported as a possible reader *and* as
// colliding with a known vendor role, and the panel shows both.
//
// The same rule applies to the sweep. The per-connect batching is verifiable and
// is implemented; the command opcodes are not, so they are supplied by the
// caller rather than invented here.

#include <cstdint>
#include <string>
#include <vector>

#include "drivers/IBleCentral.h"
#include "features/ble/HidReaderModel.h"

// One reader seen in a passive scan.
struct HidReaderCandidate {
    BleAddress address;
    std::string addressText;
    std::string name;
    int rssi = -127;
    // The 16-bit UUID the match was made on. 0 when nothing matched.
    uint16_t matchedServiceUuid = 0;
    HidReaderKind kind = HidReaderKind::Unknown;
    // Always false in this build: no authoritative source exists for either
    // mapping (X3). Carried on the candidate so the flag travels with the claim
    // instead of living in a comment somewhere.
    bool mappingVerified = false;
    // True when the same UUID is assigned to a different role by the SIG -- as
    // 0xFFF0 is to Triones / ELK-BLEDOM LED controllers. A match that collides
    // with a known role is the weakest kind of evidence there is.
    bool uuidCollidesWithAnotherRole = false;
};

// What a connect diagnostic actually established.
struct HidDiagnostic {
    bool connectSucceeded = false;
    uint16_t serviceCount = 0;
    uint16_t characteristicCount = 0;
    uint16_t notifyCharacteristicCount = 0;
    uint16_t writeCharacteristicCount = 0;
    bool subscribed = false;
    // True when a characteristic accepts a write. The reply characteristic is
    // not knowable here (X3), but the write target is: the property bits come
    // from discovery, so they are evidence rather than a guess.
    bool writableFound = false;
    bool replyCaptured = false;
    const char* detail = "not started";
};

// What a sweep pass established.
struct HidSweepResult {
    uint8_t totalCommands = 0;
    uint8_t commandsIssued = 0;
    uint8_t connectsUsed = 0;
    // Connects that produced at least one notification. A sweep that issued
    // every command and heard nothing is not a completed sweep.
    uint8_t connectsWithReply = 0;
    bool complete = false;
};

class HidReaderClient {
public:
    explicit HidReaderClient(IBleCentral& central) : central_(central) {}

    bool begin();
    // Release the controller and the subscription.
    BleStatus end();

    // ── passive scan ───────────────────────────────────────────────────────

    // Scans without requesting scan responses, which is the mode this tool is
    // specified in: a reader that wants to stay quiet keeps quiet, and a
    // detection is then something it advertised on its own.
    BleStatus passiveScan();
    const std::vector<HidReaderCandidate>& candidates() const { return candidates_; }
    uint8_t candidateCount() const {
        return static_cast<uint8_t>(candidates_.size());
    }

    // ── connect diagnostic ────────────────────────────────────────────────

    // Connect to `index`, discover, and subscribe to the first notify
    // characteristic. Fills `out` with what the reader actually exposes.
    //
    // `replyCaptured` is false on return even for a healthy reader: these
    // readers push on their own schedule, so "nothing yet" is the normal state
    // right after connecting, and reporting it as a failure would be a lie in
    // the other direction. Use replyCaptured() later.
    BleStatus diagnose(uint8_t index, HidDiagnostic& out);

    // True once any notification has arrived since the last diagnose(). A
    // reader that never answers is not a completed transaction.
    bool replyCaptured() const { return replyCaptured_; }
    uint32_t notificationsSeen() const { return notificationsSeen_; }
    // The most recent notification, for the capture log.
    const std::vector<uint8_t>& lastNotification() const { return lastNotification_; }

    // The honest end-of-transaction report, built by the model from what the
    // diagnostic established. `requested` is whether the user asked for a pass
    // at all -- an unrequested pass never reports as completed.
    HidTransactionReport report(bool requested) const;

    // ── the sweep ─────────────────────────────────────────────────────────

    // The core sweep, batched across connects by the model's scheduler so a long
    // sweep completes instead of timing out.
    //
    // `commands` is a flat buffer of `totalCommands` slices of `commandLength`
    // bytes. The opcodes come from the caller: this build has no verified
    // Signo / iCLASS SE command set, so generating one here would be inventing a
    // protocol. What is implemented is the batching and the honest accounting.
    BleStatus sweep(const uint8_t* commands, uint8_t commandLength, uint8_t totalCommands,
                    HidSweepResult& out);

    const char* lastError() const { return central_.lastError(); }

private:
    static void onNotify(void* context, const BleCharacteristicRef& ref,
                         const uint8_t* data, uint16_t length);

    // Classifies an advertisement, returning the matching service UUID or 0.
    HidReaderCandidate classifyAdvertisement(const BleAdvertisement& advertisement) const;

    IBleCentral& central_;
    std::vector<HidReaderCandidate> candidates_;
    uint8_t connectedCandidate_ = 0xFF;
    bool subscribed_ = false;
    bool replyCaptured_ = false;
    uint32_t notificationsSeen_ = 0;
    std::vector<uint8_t> lastNotification_;
    uint16_t notifyCharacteristicCount_ = 0;
    uint16_t writeCharacteristicCount_ = 0;
    // Where a command goes, and where the reply comes back. Two different
    // values: the notify characteristic is chosen by the property bits, the
    // write target likewise, and neither is asserted to be *the* one the
    // reader's protocol names.
    BleCharacteristicRef notifyRef_;
    BleCharacteristicRef writeRef_;
};
