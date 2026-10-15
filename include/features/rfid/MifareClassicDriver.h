#pragma once

// MIFARE Classic session over any IRfidTransport.
//
// This is where the Card Reader, Card Dump, Card Write and Key Check screens
// get their data. It owns activation and identification, per-sector
// authentication with an explicit key order, the full-card dump walk, and the
// writes -- including the checks that stop a write which cannot succeed from
// ever reaching the card.
//
// The transport only moves bytes. Every decision here is protocol logic, so the
// host suite drives it through a scripted transport and pins what actually
// matters:
//
//   * a sector is recovered only when an authentication succeeded,
//   * a block that could not be read is recorded as unknown, not as zeros,
//   * a write the access bits forbid never reaches the card, and
//   * a permanently locked sector is refused rather than bricked.
//
// The last point is not theoretical. Setting a trailer's C1C2C3 to 111 makes
// the sector unchangeable forever: Key A, Key B and the access bits can never
// be written again. A tool that offers "write key" without checking that first
// destroys cards. See docs/protocol-evidence/mifare-classic.md §1.3.

#include <array>
#include <cstdint>

#include "drivers/IRfidTransport.h"
#include "features/rfid/Iso14443aFraming.h"
#include "features/rfid/MifareClassicModel.h"
#include "features/rfid/MifareKeys.h"

struct MifareAuthResult {
    bool authenticated = false;
    RfidStatus status = RfidStatus::AuthFailed;
    MifareKeySlot slot = MifareKeySlot::KeyA;
    // The key that was tried, kept so a recovered dump can rebuild the sector's
    // trailer and so the report can name what opened it.
    std::array<uint8_t, kMifareKeySize> key{};
    // Index into MifareKeyTable when the key came from the published table, so
    // the UI can print the dictionary's own name for it.
    uint16_t keyIndex = MifareKeyTable::kNotFound;
    const char* detail = "not attempted";
};

struct MifareReadResult {
    RfidStatus status = RfidStatus::IoError;
    const char* detail = "not attempted";
};

struct MifareWriteResult {
    bool written = false;
    RfidStatus status = RfidStatus::IoError;
    // Names the refusal when the write was blocked before it was sent, which is
    // the difference between "the card said no" and "we never asked".
    const char* detail = "not attempted";
};

// Called after each sector is attempted during a dump walk, with the dump so
// far, so a screen can redraw without keeping its own copy in sync.
using MifareDumpProgress = void (*)(const MifareDump& dump, void* user);

class MifareClassicDriver {
public:
    explicit MifareClassicDriver(IRfidTransport& transport) : transport_(transport) {}

    // ---- activation -------------------------------------------------------
    // Polls for one card. On success the target, card type and geometry are
    // available below. Returns NoTarget when the field is empty.
    RfidStatus activate(uint32_t timeoutMs);
    // Drops the selected card and forgets the authenticated sector.
    void release();
    bool active() const { return active_; }

    const RfidTarget& target() const { return target_; }
    Iso14443aCardType cardType() const { return cardType_; }
    const char* cardTypeLabel() const;
    // Unknown for anything that is not a MIFARE Classic card; the dump screens
    // use this to refuse a card they cannot represent.
    MifareSize size() const { return size_; }
    bool supportsSectors() const { return size_ != MifareSize::Unknown; }
    // Which sector the card is currently authenticated on, or -1. MIFARE keeps
    // authentication per sector until the next authentication, so a read of
    // another sector fails -- tracking it turns that into a clear message
    // instead of a generic read error.
    int authenticatedSector() const { return authenticatedSector_; }

    // ---- authentication ---------------------------------------------------
    // One attempt, no table walk. This is the primitive Key Check drives.
    MifareAuthResult authenticate(uint8_t block,
                                 MifareKeySlot slot,
                                 const std::array<uint8_t, kMifareKeySize>& key);
    // Walks the published table for one sector, trying the NFC Forum key first
    // for each slot. Returns the first key that opens it.
    MifareAuthResult authenticateSectorWithTable(uint8_t sector);

    // ---- reads ------------------------------------------------------------
    // The block's sector must be authenticated. Reads 16 bytes.
    MifareReadResult readBlock(uint8_t block,
                               std::array<uint8_t, kMifareBlockSize>& out);
    // Authenticates the sector, then reads every block in it, recording each
    // result into `dump`: recovered blocks carry their data, the rest are
    // marked unknown so the dump never implies the read succeeded. Returns the
    // number of blocks actually read.
    uint8_t readSectorIntoDump(MifareDump& dump, uint8_t sector);
    // Walks every sector of the active card. `progress` may be null.
    MifareDump dumpCard(MifareDumpProgress progress = nullptr, void* user = nullptr);

    // ---- writes ----------------------------------------------------------
    // Writes a data block, after authenticating and checking the access bits
    // the card currently reports. Block 0 is refused: it is the manufacturer
    // block, read-only on any genuine card, and writing it via a normal write
    // is what the magic-card path exists for.
    MifareWriteResult writeBlock(uint8_t block,
                                 const std::array<uint8_t, kMifareBlockSize>& data,
                                 MifareKeySlot slot,
                                 const std::array<uint8_t, kMifareKeySize>& key);
    // Replaces a sector trailer. Refused when the sector is already permanently
    // locked, and when the access bits are inconsistent. The governing
    // permission is the access-bits write permission, because replacing a
    // trailer always rewrites the access bits as well.
    MifareWriteResult writeTrailer(uint8_t sector,
                                  const MifareTrailer& trailer,
                                  MifareKeySlot slot,
                                  const std::array<uint8_t, kMifareKeySize>& key);
    // The gen1A ("magic card") back door. Gated on both a detected back door and
    // an explicit operator confirmation, and the command sequence itself is not
    // implemented -- docs/protocol-evidence/mifare-classic.md §4 keeps it behind
    // the evidence gate. This exists so the gate has a call site instead of
    // being a comment nobody can reach.
    MifareWriteResult writeMagicUid(const std::array<uint8_t, 10>& uid,
                                   uint8_t uidLength,
                                   bool backdoorDetected,
                                   bool userConfirmed);

private:
    // Whether `slot` is permitted by an access-bits row.
    static bool allowsAccess(MifareKeyAccess access, MifareKeySlot slot);
    // Reads and decodes a sector's trailer. Requires the sector to be
    // authenticated already.
    bool readAccessBits(uint8_t sector, MifareAccessBits& bits, MifareTrailer& trailer);

    IRfidTransport& transport_;
    RfidTarget target_{};
    Iso14443aCardType cardType_ = Iso14443aCardType::Unknown;
    MifareSize size_ = MifareSize::Unknown;
    bool active_ = false;
    int authenticatedSector_ = -1;
};
