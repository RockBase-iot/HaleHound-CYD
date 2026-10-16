#pragma once

// Chameleon Ultra control model.
//
// Upstream v3.8.0 made the device a portable controller for an RRG Chameleon
// Ultra over Bluetooth: scan and connect, read HF and LF cards, clone a card
// into a slot, capture mfkey32 nonces, set up emulation, manage all eight
// slots, and decode NDEF straight off a tag. v3.9.2 added auto-connect (the
// first Chameleon found is used, instead of a pick list) and v3.9.5 made short
// reads honest: a partial NTAG read is flagged, and mfkey32 progress reports
// "N of 16" rather than implying the capture finished.

#include <cstdint>
#include <string>

class IFeature;

constexpr uint8_t kChameleonSlotCount = 8;
constexpr uint8_t kChameleonNoncesRequired = 16;

enum class ChameleonState : uint8_t {
    Idle,
    Scanning,
    Connecting,
    Ready,
    Busy,
    Failed,
};

enum class ChameleonSlotKind : uint8_t {
    Empty,
    MifareClassic1K,
    MifareClassic4K,
    MifareUltralight,
    Ntag21x,
    Iso15693,
    Em4100,
    HidProx,
    Unknown,
};

struct ChameleonSlot {
    uint8_t index = 0;
    ChameleonSlotKind kind = ChameleonSlotKind::Empty;
    std::string nickname;
    // True when a card has been read or written into this slot. A loaded slot
    // the device is not currently emulating is still loaded, which is why this
    // and `enabled` are separate answers rather than one flag.
    bool loaded = false;
    // True when the slot is switched on for emulation. The device reports this
    // per frequency (enabled_hf / enabled_lf); a slot enabled for either counts
    // as enabled, since the panel asks "is this slot live", not "on which band".
    bool enabled = false;
};

struct Mfkey32Progress {
    uint8_t nonces = 0;
    // True only once the full nonce set is in; "enough to try" is not "done".
    bool complete = false;

    uint8_t remaining() const {
        return nonces >= kChameleonNoncesRequired
                   ? 0
                   : static_cast<uint8_t>(kChameleonNoncesRequired - nonces);
    }
};

struct ChameleonReadResult {
    bool valid = false;
    bool partial = false;
    uint8_t pagesRead = 0;
    uint8_t pagesTotal = 0;
    const char* detail = "no read";
};

class ChameleonUltraModel {
public:
    static const char* slotKindLabel(ChameleonSlotKind kind);
    static const char* stateLabel(ChameleonState state);
    // A read is only "valid" when every expected page came back; a short read is
    // reported as partial so the UI can flag it instead of showing a dump as
    // if it were whole.
    static ChameleonReadResult evaluateRead(uint8_t pagesRead, uint8_t pagesTotal);
    // Adds a captured nonce and reports progress honestly.
    static Mfkey32Progress recordNonce(Mfkey32Progress progress);
    static Mfkey32Progress resetProgress();
};

class ChameleonSession {
public:
    void begin();
    void stop();

    // Auto-connect policy: the first Chameleon seen is adopted, mirroring
    // v3.9.2. Later finds do not steal the connection while one is held.
    bool onDeviceFound(const std::string& address);
    void onConnectFailed(const char* reason);

    ChameleonState state() const { return state_; }
    const std::string& address() const { return address_; }
    const char* lastError() const { return lastError_; }

    // Slot table access. Index 0..7; out-of-range indices are rejected rather
    // than wrapped, so a bad index cannot silently hit a different slot.
    bool setSlot(uint8_t index, ChameleonSlotKind kind, const std::string& nickname);
    bool clearSlot(uint8_t index);
    const ChameleonSlot& slot(uint8_t index) const;
    uint8_t loadedSlotCount() const;
    // Lowest free slot, or 0xFF when all eight are in use.
    uint8_t firstFreeSlot() const;

private:
    ChameleonState state_ = ChameleonState::Idle;
    std::string address_;
    const char* lastError_ = "";
    ChameleonSlot slots_[kChameleonSlotCount];
};

IFeature* createChameleonUltraFeature();
