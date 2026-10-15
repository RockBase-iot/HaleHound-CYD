#pragma once

// MIFARE Classic geometry, sector-trailer decode and dump assembly.
//
// This is the data plane the RFID back ends feed: given per-block reads and per
// sector authentication results, build a dump that is honest about what was
// actually recovered. Nothing here touches a radio.
//
// Access-bit packing and the (C1,C2,C3) permission tables are verified against
// four independent sources with a worked example for the factory default
// `FF 07 80 69` -- see docs/protocol-evidence/mifare-classic.md §1. That
// document also records what is deliberately NOT implemented (the magic-card
// backdoor command sequence) and why.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "features/rfid/MifareKeys.h"

constexpr uint8_t kMifareBlockSize = 16;
constexpr uint16_t kMifareMaxBlocks = 256;
constexpr uint8_t kMifareMaxSectors = 40;
// Sector trailer: Key A (0..5), access bytes (6..9), Key B (10..15).
constexpr uint8_t kMifareAccessByteOffset = 6;
constexpr uint8_t kMifareAccessByteCount = 4;
constexpr uint8_t kMifareKeyBOffset = 10;

enum class MifareSize : uint8_t {
    Mini = 0,      // MF1S20: 5 sectors / 20 blocks
    Classic1K,     // MF1S50: 16 sectors / 64 blocks
    Classic4K,     // MF1S70: 40 sectors / 256 blocks
    Unknown,
};

enum class MifareKeyAccess : uint8_t {
    None,
    KeyA,
    KeyB,
    KeyAOrB,
};

enum class MifareTrailerKeyBAccess : uint8_t {
    Readable,
    NotReadable,
    Invalid,
};

struct MifareSectorLayout {
    uint8_t index = 0;
    uint8_t firstBlock = 0;
    uint8_t blockCount = 0;
    uint8_t trailerBlock = 0;
    bool valid = false;
};

struct MifareAccessBits {
    // Nibbles straight out of the trailer; bit i of each is block i of the
    // sector (i == 3 is the trailer itself).
    uint8_t c1 = 0;
    uint8_t c2 = 0;
    uint8_t c3 = 0;
    // False when the stored inverted/normal pairs disagree. A card in that
    // state is malformed; the caller must not show permissions at all.
    bool consistent = false;

    uint8_t blockC1(uint8_t blockInSector) const { return (c1 >> blockInSector) & 0x01; }
    uint8_t blockC2(uint8_t blockInSector) const { return (c2 >> blockInSector) & 0x01; }
    uint8_t blockC3(uint8_t blockInSector) const { return (c3 >> blockInSector) & 0x01; }
};

struct MifareBlockPermission {
    MifareKeyAccess read = MifareKeyAccess::None;
    MifareKeyAccess write = MifareKeyAccess::None;
    MifareKeyAccess increment = MifareKeyAccess::None;
    // "decrement / transfer / restore" share one triplet in MF1S50.
    MifareKeyAccess decrement = MifareKeyAccess::None;
};

struct MifareTrailerPermission {
    // Key A is never readable on a real card -- the read command returns zeros
    // for the Key A bytes. Modelled explicitly so the UI cannot imply otherwise.
    MifareKeyAccess keyARead = MifareKeyAccess::None;
    MifareKeyAccess keyAWrite = MifareKeyAccess::None;
    MifareKeyAccess accessBitsRead = MifareKeyAccess::None;
    MifareKeyAccess accessBitsWrite = MifareKeyAccess::None;
    MifareKeyAccess keyBRead = MifareKeyAccess::None;
    MifareKeyAccess keyBWrite = MifareKeyAccess::None;
};

struct MifareTrailer {
    std::array<uint8_t, kMifareKeySize> keyA{};
    std::array<uint8_t, kMifareAccessByteCount> accessBytes{};
    std::array<uint8_t, kMifareKeySize> keyB{};
    MifareAccessBits access{};
    // False when the block is not a plausible trailer (inconsistent access
    // bits). Key A/B are still reported -- a partially read trailer is useful.
    bool accessValid = false;
};

struct MifareSectorKeys {
    uint8_t sector = 0;
    std::array<uint8_t, kMifareKeySize> keyA{};
    std::array<uint8_t, kMifareKeySize> keyB{};
    bool keyARecovered = false;
    bool keyBRecovered = false;
};

struct MifareDumpBlock {
    std::array<uint8_t, kMifareBlockSize> data{};
    bool known = false;
};

struct MifareDump {
    MifareSize size = MifareSize::Unknown;
    std::array<uint8_t, 10> uid{};
    uint8_t uidLength = 0;
    uint16_t atqa = 0;
    uint8_t sak = 0;
    std::vector<MifareDumpBlock> blocks;
    std::vector<MifareSectorKeys> sectors;
    uint8_t recoveredSectors = 0;
    uint8_t totalSectors = 0;

    bool complete() const {
        return totalSectors != 0 && recoveredSectors >= totalSectors;
    }
};

class MifareClassicModel {
public:
    // ---- geometry ---------------------------------------------------------
    static MifareSize sizeForBlockCount(uint16_t blocks);
    static const char* sizeLabel(MifareSize size);
    static uint8_t sectorCount(MifareSize size);
    static uint16_t blockCount(MifareSize size);
    // 4 for sectors 0..31, 16 for sectors 32..39 (the 4K "big" sectors).
    static uint8_t blocksInSector(uint8_t sector);
    static bool sectorLayout(MifareSize size, uint8_t sector, MifareSectorLayout& out);
    static uint8_t sectorOfBlock(MifareSize size, uint16_t block);
    static uint8_t trailerBlockOfSector(uint8_t sector);
    static bool isTrailerBlock(MifareSize size, uint16_t block);

    // ---- access bits (packing verified, see docs/protocol-evidence) --------
    // Decodes and checks the normal/inverted redundancy. Returns false when the
    // pairs disagree, in which case `out.consistent` is false and the permission
    // tables must not be consulted.
    static bool decodeAccessBits(const std::array<uint8_t, kMifareAccessByteCount>& bytes,
                                 MifareAccessBits& out);
    // Blocks 0..2 only; block 3 is the trailer and uses trailerPermission().
    static MifareBlockPermission blockPermission(const MifareAccessBits& bits,
                                                 uint8_t blockInSector);
    static MifareTrailerPermission trailerPermission(const MifareAccessBits& bits);
    static MifareTrailerKeyBAccess keyBAccess(const MifareAccessBits& bits);
    // Trailer triplet 111 makes the sector permanently unchangeable: Key A,
    // Key B and the access bits can never be written again.
    static bool locksSectorPermanently(const MifareAccessBits& bits);

    // ---- sector trailer ---------------------------------------------------
    static MifareTrailer parseTrailer(const std::array<uint8_t, kMifareBlockSize>& block);
    static void buildTrailer(const std::array<uint8_t, kMifareKeySize>& keyA,
                             const std::array<uint8_t, kMifareKeySize>& keyB,
                             const std::array<uint8_t, kMifareAccessByteCount>& accessBytes,
                             std::array<uint8_t, kMifareBlockSize>& out);
    // 4 access bytes of the factory transport configuration: FF 07 80 69.
    static const std::array<uint8_t, kMifareAccessByteCount>& transportAccessBytes();

    // ---- magic card (gen1A) write gate ------------------------------------
    // A UID-rewritable "magic" card is only writable through its back door when
    // the back door was actually detected AND the operator confirmed the risk.
    // Both flags are required: a genuine card must never be written through the
    // back door path. The back door command sequence itself is NOT implemented
    // (docs/protocol-evidence/mifare-classic.md §4).
    static bool magicWriteAllowed(bool backdoorDetected, bool userConfirmed);

    // ---- dump assembly ----------------------------------------------------
    static bool beginDump(MifareDump& dump,
                          MifareSize size,
                          uint8_t uidLength,
                          const std::array<uint8_t, 10>& uid,
                          uint16_t atqa,
                          uint8_t sak);
    static bool setBlock(MifareDump& dump,
                         uint16_t block,
                         const std::array<uint8_t, kMifareBlockSize>& data);
    static bool markBlockUnknown(MifareDump& dump, uint16_t block);
    static bool setSectorKeyA(MifareDump& dump,
                              uint8_t sector,
                              const std::array<uint8_t, kMifareKeySize>& key);
    static bool setSectorKeyB(MifareDump& dump,
                              uint8_t sector,
                              const std::array<uint8_t, kMifareKeySize>& key);
    static uint16_t unknownBlockCount(const MifareDump& dump);
    // UID as the number every MIFARE tool prints: the on-air order is LSB
    // first, so uid[3]<<24 | uid[2]<<16 | uid[1]<<8 | uid[0].
    static uint32_t uid32(const MifareDump& dump);
    // Human label for honest reporting, e.g. "12 of 16 sectors recovered".
    static std::string recoverySummary(const MifareDump& dump);
};
