#pragma once

#include <cstdint>

// SD card ownership across a radio session.
//
// Bench symptom this exists for: run an RFID/SubGHz/NRF feature, back out, open
// Loot — the card reads as empty. The radio never handed the shared SPI bus
// back in a state where the SD volume was mounted, and the Loot reader reported
// "no captures" instead of "card not mounted". Silent-empty is the bug; a
// distinct Busy/Remounted code is the fix.
//
// The model is deliberately pure: mount/unmount are counters, not SD.begin().
enum class SdState : uint8_t {
    Unmounted = 0,
    Mounted = 1,
    RadioOwnsBus = 2,
};

enum class SdCode : uint8_t {
    Ready,       // mounted and valid, no action taken
    Remounted,   // the card had to be re-mounted before this call could proceed
    Busy,        // the radio still owns the shared bus — retry, do not report empty
    NotMounted,  // no card / mount failed
    Invalid,     // bad request (null or empty path, wrong state)
};

struct SdHandoffResult {
    SdCode code;
    const char* detail;
};

class SdHandoff {
public:
    explicit SdHandoff(bool cardPresent = true) : cardPresent_(cardPresent) {}

    // Radio feature takes the shared bus. Unmounts the volume first, exactly
    // like the real driver must, so no half-open FAT state survives.
    SdCode beginRadioSession();
    // Radio feature releases the bus. The volume stays unmounted: re-mounting
    // is the next reader's job, which is what makes the handoff explicit.
    SdCode endRadioSession();

    SdCode mount();
    SdCode unmount();

    // The Loot read path. Never returns Ready when the volume is not actually
    // mounted; auto-remounts once and reports Remounted so the caller knows a
    // remount happened.
    SdHandoffResult readForLoot(const char* path);

    SdState state() const { return state_; }
    uint32_t mountCount() const { return mountCount_; }
    uint32_t remountCount() const { return remountCount_; }
    uint32_t blockedReadCount() const { return blockedReadCount_; }
    const char* lastDetail() const { return lastDetail_; }

private:
    SdHandoffResult fail(SdCode code, const char* detail);

    bool cardPresent_;
    SdState state_ = SdState::Unmounted;
    uint32_t mountCount_ = 0;
    uint32_t remountCount_ = 0;
    uint32_t blockedReadCount_ = 0;
    const char* lastDetail_ = "never mounted";
};
