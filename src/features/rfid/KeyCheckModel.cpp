#include "features/rfid/KeyCheckModel.h"

#include <cstdio>

namespace {

// Table indices of the NFC Forum keys. Kept as named constants so a reorder of
// the key table cannot silently repoint the hint at a different key -- the
// static_assert in MifareKeys.cpp already pins the table size, and the unit
// tests pin these two values.
constexpr uint16_t kMadKeyAIndex = 2;          // A0 A1 A2 A3 A4 A5
constexpr uint16_t kNdefSectorKeyAIndex = 23;  // D3 F7 D3 F7 D3 F7

}  // namespace

uint32_t KeyCheckSession::maxAttempts(uint8_t sectors) {
    // Both slots, whole table, per sector. Once a sector authenticates the scan
    // stops early, so this is an upper bound -- which is the honest thing to
    // divide by when the operator wants "how much longer".
    return static_cast<uint32_t>(sectors) * 2u * static_cast<uint32_t>(MifareKeyTable::count());
}

uint16_t KeyCheckSession::preferredKeyIndex(MifareKeySlot slot, uint8_t sector) {
    if (slot == MifareKeySlot::KeyA) {
        // Sector 0 of an NDEF card is the MAD sector (key A = A0A1A2A3A4A5);
        // NDEF data sectors use D3F7D3F7D3F7.
        return sector == 0 ? kMadKeyAIndex : kNdefSectorKeyAIndex;
    }
    // Key B has no NDEF convention; the factory transport key is the best guess.
    return 0;
}

bool KeyCheckSession::begin(uint8_t sectors) {
    if (sectors == 0 || sectors > kKeyCheckMaxSectors) {
        return false;
    }
    report_ = KeyCheckReport{};
    results_.fill(KeyCheckSectorResult{});
    sectorsTotal_ = sectors;
    for (uint8_t sector = 0; sector < sectors; ++sector) {
        results_[sector].sector = sector;
    }
    report_.state = KeyCheckState::Running;
    report_.sectorsTotal = sectors;
    report_.maxAttempts = maxAttempts(sectors);
    report_.detail = "scanning published keys";
    currentSector_ = 0;
    return true;
}

void KeyCheckSession::cancel() {
    if (report_.state == KeyCheckState::Running) {
        report_.state = KeyCheckState::Cancelled;
        report_.detail = "cancelled by operator";
    }
}

void KeyCheckSession::onAttempt(MifareKeySlot slot, uint16_t keyIndex, bool authenticated) {
    if (report_.state != KeyCheckState::Running || currentSector_ == 0xFF) {
        return;
    }
    // Every attempt counts, including the ones that fail: the counter drives the
    // progress bar and must never lag behind reality.
    ++report_.attempts;

    if (!authenticated) {
        return;
    }

    KeyCheckSectorResult& result = results_[currentSector_];
    // Guard against a double report for the same sector, which would inflate the
    // recovered count and make "N of M" meaningless.
    if (result.outcome != KeyCheckSectorOutcome::Recovered) {
        result.outcome = KeyCheckSectorOutcome::Recovered;
        result.slot = slot;
        result.keyIndex = keyIndex;
        ++report_.sectorsRecovered;
    }
    advanceToNextUntried();
}

void KeyCheckSession::onSectorExhausted() {
    if (report_.state != KeyCheckState::Running || currentSector_ == 0xFF) {
        return;
    }
    KeyCheckSectorResult& result = results_[currentSector_];
    if (result.outcome == KeyCheckSectorOutcome::Untried) {
        result.outcome = KeyCheckSectorOutcome::Exhausted;
        ++report_.sectorsExhausted;
    }
    advanceToNextUntried();
}

void KeyCheckSession::advanceToNextUntried() {
    for (uint8_t sector = 0; sector < sectorsTotal_; ++sector) {
        if (results_[sector].outcome == KeyCheckSectorOutcome::Untried) {
            currentSector_ = sector;
            return;
        }
    }
    currentSector_ = 0xFF;
    report_.state = KeyCheckState::Finished;
    // "Finished" says the scan ended, never that the card was opened. The
    // summary carries that distinction.
    report_.detail = report_.allRecovered() ? "every sector recovered"
                                            : "finished with unrecovered sectors";
}

const KeyCheckSectorResult* KeyCheckSession::sectorResult(uint8_t sector) const {
    if (sector >= sectorsTotal_) {
        return nullptr;
    }
    return &results_[sector];
}

uint8_t KeyCheckReport::progressPercent() const {
    if (maxAttempts == 0) {
        return 0;
    }
    if (attempts >= maxAttempts) {
        return 100;
    }
    return static_cast<uint8_t>((attempts * 100u) / maxAttempts);
}

std::string KeyCheckReport::summary() const {
    char buffer[128];
    if (state == KeyCheckState::Idle) {
        return "key check not started";
    }
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%u of %u sectors recovered",
                  static_cast<unsigned>(sectorsRecovered),
                  static_cast<unsigned>(sectorsTotal));
    std::string summary(buffer);
    if (sectorsExhausted != 0) {
        char tail[64];
        std::snprintf(tail,
                      sizeof(tail),
                      ", %u need a nested/hardnested attack",
                      static_cast<unsigned>(sectorsExhausted));
        summary += tail;
    }
    return summary;
}
