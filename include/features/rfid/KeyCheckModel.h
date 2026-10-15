#pragma once

// Key Check: try the published default keys against every sector, and report
// honestly what actually authenticated.
//
// The value of this module is the reporting, not the loop. Two failure modes it
// exists to prevent:
//
//   1. Calling a sector "cracked" because a key was *tried*. A sector is only
//      recovered once an authentication actually succeeded, and the key that
//      opened it is recorded.
//   2. Reporting progress as a fraction of sectors. That sits at 0 % for the
//      first ~58 attempts and then jumps, so the operator cannot tell a slow
//      scan from a hung one. Progress is therefore derived from the attempt
//      counter, while the headline number stays "N of M sectors".
//
// A sector that no published key opens is reported as `Exhausted` -- it needs a
// nested / hardnested attack -- rather than as a failure. That distinction is
// what tells the operator whether to reach for the stronger attack or to stop.

#include <array>
#include <cstdint>
#include <string>

#include "features/rfid/MifareKeys.h"

// A 4K card has 40 sectors; a Mini has 5 and a 1K has 16, so 40 covers them all.
constexpr uint8_t kKeyCheckMaxSectors = 40;

enum class KeyCheckState : uint8_t {
    Idle,
    Running,
    Finished,  // every started sector reached a terminal outcome
    Cancelled,
};

enum class KeyCheckSectorOutcome : uint8_t {
    Untried,
    Recovered,  // a published key authenticated
    Exhausted,  // every published key was tried and rejected
};

struct KeyCheckSectorResult {
    uint8_t sector = 0;
    KeyCheckSectorOutcome outcome = KeyCheckSectorOutcome::Untried;
    MifareKeySlot slot = MifareKeySlot::KeyA;
    uint16_t keyIndex = 0;  // index into MifareKeyTable
};

struct KeyCheckReport {
    KeyCheckState state = KeyCheckState::Idle;
    uint8_t sectorsTotal = 0;
    uint8_t sectorsRecovered = 0;
    uint8_t sectorsExhausted = 0;
    uint32_t attempts = 0;
    uint32_t maxAttempts = 0;
    const char* detail = "not started";

    bool allRecovered() const {
        return sectorsTotal != 0 && sectorsRecovered >= sectorsTotal;
    }

    // Attempt-based, because that is the only quantity that advances smoothly.
    uint8_t progressPercent() const;

    // e.g. "12 of 16 sectors recovered, 4 need a nested attack".
    std::string summary() const;
};

class KeyCheckSession {
public:
    // Worst case: every sector probes both key slots against the whole table.
    static uint32_t maxAttempts(uint8_t sectors);

    // Which key to try *first* for a given slot/sector. On an NDEF-formatted
    // card this is the documented NFC Forum key (MAD key A in sector 0, the
    // NDEF data-sector key A elsewhere), which saves walking the table. It is a
    // hint only -- the scan still falls through to the full table.
    static uint16_t preferredKeyIndex(MifareKeySlot slot, uint8_t sector);

    bool begin(uint8_t sectors);
    void cancel();

    // Record one authentication attempt against the current sector. On success
    // the sector is marked recovered and the session moves to the next untried
    // sector.
    void onAttempt(MifareKeySlot slot, uint16_t keyIndex, bool authenticated);
    // The driver has tried every key in the table on the current sector.
    void onSectorExhausted();

    const KeyCheckReport& report() const { return report_; }
    uint8_t sectorsTotal() const { return sectorsTotal_; }
    // 0xFF once no sector is left to try.
    uint8_t currentSector() const { return currentSector_; }
    // nullptr when `sector` is out of range for this card.
    const KeyCheckSectorResult* sectorResult(uint8_t sector) const;

private:
    void advanceToNextUntried();

    KeyCheckReport report_;
    uint8_t sectorsTotal_ = 0;
    uint8_t currentSector_ = 0xFF;
    std::array<KeyCheckSectorResult, kKeyCheckMaxSectors> results_{};
};
