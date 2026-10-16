#include "features/ble/HidReaderClient.h"

#include "features/ble/GattNameTable.h"

namespace {

// Passive, as the tool is specified: the reader answers only what it chose to
// advertise, so a detection is the reader's own statement rather than the
// result of poking it.
constexpr bool kPassiveScan = false;

// The cap on a scan list, for the same reason the other clients have one.
constexpr std::size_t kMaxCandidates = 16;
constexpr uint8_t kMaxScanPages = 32;

}  // namespace

bool HidReaderClient::begin() {
    candidates_.clear();
    connectedCandidate_ = 0xFF;
    subscribed_ = false;
    replyCaptured_ = false;
    notificationsSeen_ = 0;
    lastNotification_.clear();
    notifyCharacteristicCount_ = 0;
    notifyRef_ = BleCharacteristicRef{};
    writeRef_ = BleCharacteristicRef{};
    return central_.begin();
}

BleStatus HidReaderClient::end() {
    if (central_.scanning()) {
        central_.stopScan();
    }
    if (subscribed_) {
        central_.unsubscribe(notifyRef_);
        subscribed_ = false;
    }
    central_.end();
    candidates_.clear();
    connectedCandidate_ = 0xFF;
    // The refs go too: leaving a stale write target behind would let a later
    // sweep look addressable when the controller it belonged to is gone.
    notifyRef_ = BleCharacteristicRef{};
    writeRef_ = BleCharacteristicRef{};
    return BleStatus::Ok;
}

// ── classification ─────────────────────────────────────────────────────────

HidReaderCandidate HidReaderClient::classifyAdvertisement(
    const BleAdvertisement& advertisement) const {
    HidReaderCandidate candidate;
    candidate.address = advertisement.address;
    char text[18] = {};
    if (advertisement.address.write(text, sizeof(text))) {
        candidate.addressText = text;
    }
    candidate.name = advertisement.name;
    candidate.rssi = advertisement.rssi;

    for (uint8_t index = 0; index < advertisement.serviceCount; ++index) {
        const BleUuid& uuid = advertisement.serviceUuids[index];
        // Only a SIG-base UUID has a 16-bit form, and the reader numbers are
        // 16-bit. A 128-bit vendor UUID is not one of these, and forcing it
        // through shortValue() would give 0 and a bogus match on 0xFFF0's
        // absence.
        if (!uuid.isShort()) {
            continue;
        }
        const uint16_t value = uuid.shortValue();
        const HidReaderKind kind = HidReaderModel::classify(value);
        if (kind == HidReaderKind::Unknown) {
            continue;
        }
        candidate.matchedServiceUuid = value;
        candidate.kind = kind;
        // No authoritative source exists for either mapping, so nothing this
        // build matches on is verified (X3).
        candidate.mappingVerified = false;
        // Independent cross-check against the SIG table this project already
        // trusts: if the same number names a different role there, the reader
        // match is the weaker claim of the two. The table's contract is an
        // *empty string* for an unlisted UUID, never a null pointer -- testing
        // for null would flag every match as colliding.
        const char* knownRole = GattNameTable::serviceName(value);
        candidate.uuidCollidesWithAnotherRole = knownRole != nullptr && knownRole[0] != '\0';
        break;
    }
    return candidate;
}

// ── passive scan ───────────────────────────────────────────────────────────

BleStatus HidReaderClient::passiveScan() {
    candidates_.clear();
    const BleStatus started = central_.startScan(0, kPassiveScan);
    if (started != BleStatus::Ok) {
        return started;
    }
    BleAdvertisement batch[kBleMaxAdvServices];
    for (uint8_t page = 0; page < kMaxScanPages; ++page) {
        const uint8_t count = central_.scanResults(batch, kBleMaxAdvServices);
        if (count == 0U) {
            break;
        }
        for (uint8_t index = 0; index < count; ++index) {
            const BleAdvertisement& advertisement = batch[index];
            if (!advertisement.connectable || !advertisement.address.valid()) {
                continue;
            }
            const HidReaderCandidate candidate = classifyAdvertisement(advertisement);
            if (candidate.matchedServiceUuid == 0U) {
                // Advertised nothing this build recognises. It is not a reader
                // as far as the evidence goes, so it is not listed as one.
                continue;
            }
            bool seen = false;
            for (const HidReaderCandidate& existing : candidates_) {
                if (existing.address == advertisement.address) {
                    seen = true;
                    break;
                }
            }
            if (seen || candidates_.size() >= kMaxCandidates) {
                continue;
            }
            candidates_.push_back(candidate);
        }
        if (candidates_.size() >= kMaxCandidates) {
            break;
        }
    }
    central_.stopScan();
    return BleStatus::Ok;
}

// ── connect diagnostic ────────────────────────────────────────────────────

void HidReaderClient::onNotify(void* context, const BleCharacteristicRef& ref,
                               const uint8_t* data, uint16_t length) {
    (void)ref;
    if (context == nullptr) {
        return;
    }
    HidReaderClient* self = static_cast<HidReaderClient*>(context);
    ++self->notificationsSeen_;
    self->replyCaptured_ = true;
    self->lastNotification_.assign(data, data + length);
}

BleStatus HidReaderClient::diagnose(uint8_t index, HidDiagnostic& out) {
    out = HidDiagnostic{};
    if (index >= candidates_.size()) {
        out.detail = "no such candidate";
        return BleStatus::NotFound;
    }
    replyCaptured_ = false;
    notificationsSeen_ = 0;
    lastNotification_.clear();
    notifyCharacteristicCount_ = 0;
    notifyRef_ = BleCharacteristicRef{};
    writeRef_ = BleCharacteristicRef{};

    const HidReaderCandidate& candidate = candidates_[index];
    const BleStatus connected = central_.connect(candidate.address, 2000);
    if (connected != BleStatus::Ok) {
        out.detail = bleStatusLabel(connected);
        return connected;
    }
    out.connectSucceeded = true;

    BleDiscoveredPeer peer;
    const BleStatus discovered = central_.discover(peer);
    if (discovered != BleStatus::Ok) {
        out.detail = bleStatusLabel(discovered);
        central_.disconnect();
        return discovered;
    }
    out.serviceCount = peer.serviceCount;

    // Subscribe to the first notifying characteristic. Which characteristic
    // carries the reply is exactly the part of this protocol that has no
    // authoritative source (X3), so the diagnostic reports how many candidates
    // there were rather than asserting it found *the* one.
    bool haveNotify = false;
    bool haveWrite = false;
    for (uint8_t serviceIndex = 0; serviceIndex < peer.serviceCount; ++serviceIndex) {
        const BleDiscoveredService& service = peer.services[serviceIndex];
        for (uint8_t charIndex = 0; charIndex < service.characteristicCount; ++charIndex) {
            const BleDiscoveredCharacteristic& characteristic = service.characteristics[charIndex];
            out.characteristicCount = static_cast<uint16_t>(out.characteristicCount + 1U);
            BleCharacteristicRef ref;
            ref.service = service.uuid;
            ref.characteristic = characteristic.uuid;

            const bool canWrite =
                (characteristic.properties & (kBlePropWrite | kBlePropWriteNoResponse)) != 0U;
            if (canWrite) {
                ++writeCharacteristicCount_;
                if (!haveWrite) {
                    writeRef_ = ref;
                    haveWrite = true;
                }
            }

            if ((characteristic.properties & (kBlePropNotify | kBlePropIndicate)) == 0U) {
                continue;
            }
            ++notifyCharacteristicCount_;
            if (haveNotify) {
                continue;
            }
            const BleStatus subscribed =
                central_.subscribe(ref, &HidReaderClient::onNotify, this);
            if (subscribed == BleStatus::Ok) {
                notifyRef_ = ref;
                subscribed_ = true;
                haveNotify = true;
                out.subscribed = true;
            }
        }
    }
    out.notifyCharacteristicCount = notifyCharacteristicCount_;
    out.writeCharacteristicCount = writeCharacteristicCount_;
    if (!haveWrite) {
        writeRef_ = BleCharacteristicRef{};
    }
    out.writableFound = haveWrite;
    out.replyCaptured = replyCaptured_;

    if (!haveNotify || !haveWrite) {
        // Either half missing means the transaction cannot complete -- no reply
        // can ever be captured without a subscription, and no command can be
        // sent without a write target. Saying which half is missing here beats
        // letting the sweep issue commands into a void and time out.
        out.detail = !haveNotify
                         ? (haveWrite ? "connected; reader offers nothing to notify on"
                                      : "connected; reader offers no notify or write characteristic")
                         : "connected; reader offers nothing to write to";
        connectedCandidate_ = index;
        return BleStatus::NoService;
    }
    out.detail = "connected; listening for the reader's reply";
    connectedCandidate_ = index;
    return BleStatus::Ok;
}

HidTransactionReport HidReaderClient::report(bool requested) const {
    // The model decides what the three facts add up to, so the "no reply is not
    // a completion" rule lives in one place and is already tested there.
    return HidReaderModel::report(requested, connectedCandidate_ != 0xFF, replyCaptured_);
}

// ── the sweep ─────────────────────────────────────────────────────────────

BleStatus HidReaderClient::sweep(const uint8_t* commands, uint8_t commandLength,
                                 uint8_t totalCommands, HidSweepResult& out) {
    out = HidSweepResult{};
    out.totalCommands = totalCommands;
    if (totalCommands == 0U) {
        // Nothing to sweep is not a completed sweep.
        return BleStatus::Ok;
    }
    if (commands == nullptr || commandLength == 0U) {
        return BleStatus::IoError;
    }
    if (connectedCandidate_ == 0xFF || !subscribed_) {
        return BleStatus::Disconnected;
    }
    if (!writeRef_.valid()) {
        // Nothing to write to. A sweep that reported success here would be
        // counting writes that never happened.
        return BleStatus::NoService;
    }

    HidSweepScheduler scheduler;
    scheduler.begin(totalCommands);
    uint8_t batch[kHidSweepCommandsPerConnect] = {};
    for (;;) {
        const uint8_t count = scheduler.nextBatch(batch, kHidSweepCommandsPerConnect);
        if (count == 0U) {
            break;
        }
        replyCaptured_ = false;
        for (uint8_t index = 0; index < count; ++index) {
            const uint8_t command = batch[index];
            if (command >= totalCommands) {
                break;
            }
            const uint8_t* payload = commands + static_cast<std::size_t>(command) * commandLength;
            const BleStatus written =
                central_.write(writeRef_, payload, commandLength, false);
            if (written != BleStatus::Ok) {
                // A refused write is not a swept command. The counters are left
                // where they stand rather than tidied: the caller can see that
                // the pass stopped part way, which is the fact that matters.
                if (replyCaptured_) {
                    ++out.connectsWithReply;
                }
                ++out.connectsUsed;
                return written;
            }
            ++out.commandsIssued;
        }
        if (replyCaptured_) {
            ++out.connectsWithReply;
        }
        ++out.connectsUsed;
    }
    out.complete = out.commandsIssued == totalCommands && out.connectsWithReply > 0U;
    return BleStatus::Ok;
}
