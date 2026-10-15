#include "features/rfid/MifareClassicModel.h"

#include <cstdio>

namespace {

// Data-block permission table, indexed by (C1 << 2) | (C2 << 1) | C3.
// Rows in the datasheet order: 000 010 100 110 001 011 101 111.
struct DataRow {
    MifareKeyAccess read;
    MifareKeyAccess write;
    MifareKeyAccess increment;
    MifareKeyAccess decrement;
};

constexpr DataRow kDataRows[8] = {
    /* 000 */ {MifareKeyAccess::KeyAOrB, MifareKeyAccess::KeyAOrB, MifareKeyAccess::KeyAOrB, MifareKeyAccess::KeyAOrB},
    /* 001 */ {MifareKeyAccess::KeyAOrB, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::KeyAOrB},
    /* 010 */ {MifareKeyAccess::KeyAOrB, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::None},
    /* 011 */ {MifareKeyAccess::KeyB, MifareKeyAccess::KeyB, MifareKeyAccess::None, MifareKeyAccess::None},
    /* 100 */ {MifareKeyAccess::KeyAOrB, MifareKeyAccess::KeyB, MifareKeyAccess::None, MifareKeyAccess::None},
    /* 101 */ {MifareKeyAccess::KeyB, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::None},
    /* 110 */ {MifareKeyAccess::KeyAOrB, MifareKeyAccess::KeyB, MifareKeyAccess::KeyB, MifareKeyAccess::KeyAOrB},
    /* 111 */ {MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::None},
};

// Sector-trailer permission table, indexed by (C1_3 << 2) | (C2_3 << 1) | C3_3,
// i.e. listed in C1C2C3 order 000 001 010 011 100 101 110 111.
//
// Note the asymmetry with kDataRows above: on a trailer the columns are
// *Key A only* or *Key B only* -- never "Key A or Key B". A card never accepts
// Key B to write Key A, so collapsing these to KeyAOrB would tell the operator
// they can re-key a sector with a key they do not actually hold.
//
// Two invariants worth stating because they are easy to get wrong:
//   * Key A is never readable -- the read returns six zero bytes. Modelled as
//     None so the UI cannot render those zeroes as the key.
//   * "Access bits read" is Key A for every single row. There is no
//     configuration in which Key B may read the access bits.
struct TrailerRow {
    MifareKeyAccess keyARead;
    MifareKeyAccess keyAWrite;
    MifareKeyAccess accessRead;
    MifareKeyAccess accessWrite;
    MifareKeyAccess keyBRead;
    MifareKeyAccess keyBWrite;
};

constexpr TrailerRow kTrailerRows[8] = {
    /* 000 */ {MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::KeyA, MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::KeyA},
    /* 001 */ {MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::KeyA, MifareKeyAccess::KeyA, MifareKeyAccess::KeyA, MifareKeyAccess::KeyA},
    /* 010 */ {MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::None},
    /* 011 */ {MifareKeyAccess::None, MifareKeyAccess::KeyB, MifareKeyAccess::KeyA, MifareKeyAccess::KeyB, MifareKeyAccess::None, MifareKeyAccess::KeyB},
    /* 100 */ {MifareKeyAccess::None, MifareKeyAccess::KeyB, MifareKeyAccess::KeyA, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::KeyB},
    /* 101 */ {MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::KeyB, MifareKeyAccess::None, MifareKeyAccess::None},
    /* 110 */ {MifareKeyAccess::None, MifareKeyAccess::KeyB, MifareKeyAccess::KeyA, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::None},
    /* 111 */ {MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::KeyA, MifareKeyAccess::None, MifareKeyAccess::None, MifareKeyAccess::None},
};

constexpr uint8_t kNibbleMask = 0x0F;

uint8_t lowNibble(uint8_t byte) { return static_cast<uint8_t>(byte & kNibbleMask); }
uint8_t highNibble(uint8_t byte) { return static_cast<uint8_t>((byte >> 4) & kNibbleMask); }
uint8_t invertNibble(uint8_t nibble) { return static_cast<uint8_t>(~nibble & kNibbleMask); }

// Factory transport configuration: Key A = FF*6, access bytes = FF 07 80 69,
// Key B = FF*6. byte 9 (0x69) is the reserved byte and is not decoded.
constexpr std::array<uint8_t, kMifareAccessByteCount> kTransportAccessBytes = {0xFF, 0x07, 0x80, 0x69};

int rowIndex(uint8_t c1, uint8_t c2, uint8_t c3) {
    return static_cast<int>((c1 << 2) | (c2 << 1) | c3);
}

}  // namespace

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

MifareSize MifareClassicModel::sizeForBlockCount(uint16_t blocks) {
    if (blocks == 20) {
        return MifareSize::Mini;
    }
    if (blocks == 64) {
        return MifareSize::Classic1K;
    }
    if (blocks == 256) {
        return MifareSize::Classic4K;
    }
    return MifareSize::Unknown;
}

const char* MifareClassicModel::sizeLabel(MifareSize size) {
    switch (size) {
        case MifareSize::Mini: return "Mini";
        case MifareSize::Classic1K: return "1K";
        case MifareSize::Classic4K: return "4K";
        default: return "Unknown";
    }
}

uint8_t MifareClassicModel::sectorCount(MifareSize size) {
    switch (size) {
        case MifareSize::Mini: return 5;
        case MifareSize::Classic1K: return 16;
        case MifareSize::Classic4K: return 40;
        default: return 0;
    }
}

uint16_t MifareClassicModel::blockCount(MifareSize size) {
    switch (size) {
        case MifareSize::Mini: return 20;
        case MifareSize::Classic1K: return 64;
        case MifareSize::Classic4K: return 256;
        default: return 0;
    }
}

uint8_t MifareClassicModel::blocksInSector(uint8_t sector) {
    // Sectors 32..39 of a 4K card are four times the size of the first 32.
    return sector < 32 ? 4 : 16;
}

bool MifareClassicModel::sectorLayout(MifareSize size, uint8_t sector, MifareSectorLayout& out) {
    out = MifareSectorLayout{};
    if (sector >= sectorCount(size)) {
        return false;
    }
    const uint8_t count = blocksInSector(sector);
    out.index = sector;
    out.blockCount = count;
    out.firstBlock = sector < 32
                         ? static_cast<uint8_t>(sector * 4)
                         : static_cast<uint8_t>(128 + (sector - 32) * 16);
    out.trailerBlock = static_cast<uint8_t>(out.firstBlock + count - 1);
    out.valid = true;
    return true;
}

uint8_t MifareClassicModel::sectorOfBlock(MifareSize size, uint16_t block) {
    if (block >= blockCount(size)) {
        return 0xFF;
    }
    if (block < 128) {
        return static_cast<uint8_t>(block / 4);
    }
    return static_cast<uint8_t>(32 + (block - 128) / 16);
}

uint8_t MifareClassicModel::trailerBlockOfSector(uint8_t sector) {
    return sector < 32 ? static_cast<uint8_t>(sector * 4 + 3)
                       : static_cast<uint8_t>(128 + (sector - 32) * 16 + 15);
}

bool MifareClassicModel::isTrailerBlock(MifareSize size, uint16_t block) {
    if (block >= blockCount(size)) {
        return false;
    }
    const uint8_t sector = sectorOfBlock(size, block);
    return sector != 0xFF && block == trailerBlockOfSector(sector);
}

// ---------------------------------------------------------------------------
// Access bits
// ---------------------------------------------------------------------------

bool MifareClassicModel::decodeAccessBits(const std::array<uint8_t, kMifareAccessByteCount>& bytes,
                                          MifareAccessBits& out) {
    // Packing (docs/protocol-evidence/mifare-classic.md §1.1):
    //   byte 6 = ~C2 << 4 | ~C1
    //   byte 7 =  C1 << 4 | ~C3
    //   byte 8 =  C3 << 4 |  C2
    //   byte 9 =  reserved, not decoded
    const uint8_t byte6 = bytes[0];
    const uint8_t byte7 = bytes[1];
    const uint8_t byte8 = bytes[2];

    const uint8_t c1 = highNibble(byte7);
    const uint8_t c2 = lowNibble(byte8);
    const uint8_t c3 = highNibble(byte8);

    out.c1 = c1;
    out.c2 = c2;
    out.c3 = c3;

    // Each nibble is stored twice, once inverted. Any mismatch means we are
    // misreading the trailer; reporting permissions from it would be a guess.
    const bool c1Ok = lowNibble(byte6) == invertNibble(c1);
    const bool c2Ok = highNibble(byte6) == invertNibble(c2);
    const bool c3Ok = lowNibble(byte7) == invertNibble(c3);
    out.consistent = c1Ok && c2Ok && c3Ok;
    return out.consistent;
}

MifareBlockPermission MifareClassicModel::blockPermission(const MifareAccessBits& bits,
                                                          uint8_t blockInSector) {
    MifareBlockPermission permission;
    if (!bits.consistent || blockInSector > 2) {
        return permission;
    }
    const DataRow& row = kDataRows[rowIndex(bits.blockC1(blockInSector),
                                            bits.blockC2(blockInSector),
                                            bits.blockC3(blockInSector))];
    permission.read = row.read;
    permission.write = row.write;
    permission.increment = row.increment;
    permission.decrement = row.decrement;
    return permission;
}

MifareTrailerPermission MifareClassicModel::trailerPermission(const MifareAccessBits& bits) {
    MifareTrailerPermission permission;
    if (!bits.consistent) {
        return permission;
    }
    const TrailerRow& row = kTrailerRows[rowIndex(bits.blockC1(3), bits.blockC2(3), bits.blockC3(3))];
    permission.keyARead = row.keyARead;
    permission.keyAWrite = row.keyAWrite;
    permission.accessBitsRead = row.accessRead;
    permission.accessBitsWrite = row.accessWrite;
    permission.keyBRead = row.keyBRead;
    permission.keyBWrite = row.keyBWrite;
    return permission;
}

MifareTrailerKeyBAccess MifareClassicModel::keyBAccess(const MifareAccessBits& bits) {
    if (!bits.consistent) {
        return MifareTrailerKeyBAccess::Invalid;
    }
    const TrailerRow& row = kTrailerRows[rowIndex(bits.blockC1(3), bits.blockC2(3), bits.blockC3(3))];
    return row.keyBRead == MifareKeyAccess::None ? MifareTrailerKeyBAccess::NotReadable
                                                 : MifareTrailerKeyBAccess::Readable;
}

bool MifareClassicModel::locksSectorPermanently(const MifareAccessBits& bits) {
    if (!bits.consistent) {
        return false;
    }
    // Trailer triplet 111: Key A write, access-bit write and Key B write are all
    // "never", so the sector can never be re-keyed or unlocked again.
    return bits.blockC1(3) == 1 && bits.blockC2(3) == 1 && bits.blockC3(3) == 1;
}

// ---------------------------------------------------------------------------
// Sector trailer
// ---------------------------------------------------------------------------

MifareTrailer MifareClassicModel::parseTrailer(const std::array<uint8_t, kMifareBlockSize>& block) {
    MifareTrailer trailer;
    for (uint8_t i = 0; i < kMifareKeySize; ++i) {
        trailer.keyA[i] = block[i];
        trailer.keyB[i] = block[kMifareKeyBOffset + i];
    }
    for (uint8_t i = 0; i < kMifareAccessByteCount; ++i) {
        trailer.accessBytes[i] = block[kMifareAccessByteOffset + i];
    }
    trailer.accessValid = decodeAccessBits(trailer.accessBytes, trailer.access);
    return trailer;
}

void MifareClassicModel::buildTrailer(const std::array<uint8_t, kMifareKeySize>& keyA,
                                      const std::array<uint8_t, kMifareKeySize>& keyB,
                                      const std::array<uint8_t, kMifareAccessByteCount>& accessBytes,
                                      std::array<uint8_t, kMifareBlockSize>& out) {
    for (uint8_t i = 0; i < kMifareKeySize; ++i) {
        out[i] = keyA[i];
        out[kMifareKeyBOffset + i] = keyB[i];
    }
    for (uint8_t i = 0; i < kMifareAccessByteCount; ++i) {
        out[kMifareAccessByteOffset + i] = accessBytes[i];
    }
}

const std::array<uint8_t, kMifareAccessByteCount>& MifareClassicModel::transportAccessBytes() {
    return kTransportAccessBytes;
}

// ---------------------------------------------------------------------------
// Magic card write gate
// ---------------------------------------------------------------------------

bool MifareClassicModel::magicWriteAllowed(bool backdoorDetected, bool userConfirmed) {
    return backdoorDetected && userConfirmed;
}

// ---------------------------------------------------------------------------
// Dump assembly
// ---------------------------------------------------------------------------

bool MifareClassicModel::beginDump(MifareDump& dump,
                                   MifareSize size,
                                   uint8_t uidLength,
                                   const std::array<uint8_t, 10>& uid,
                                   uint16_t atqa,
                                   uint8_t sak) {
    dump = MifareDump{};
    if (size == MifareSize::Unknown || uidLength == 0 || uidLength > uid.size()) {
        return false;
    }
    dump.size = size;
    dump.uid = uid;
    dump.uidLength = uidLength;
    dump.atqa = atqa;
    dump.sak = sak;
    dump.blocks.assign(blockCount(size), MifareDumpBlock{});
    dump.sectors.assign(sectorCount(size), MifareSectorKeys{});
    for (uint8_t sector = 0; sector < sectorCount(size); ++sector) {
        dump.sectors[sector].sector = sector;
    }
    dump.totalSectors = sectorCount(size);
    return true;
}

bool MifareClassicModel::setBlock(MifareDump& dump,
                                  uint16_t block,
                                  const std::array<uint8_t, kMifareBlockSize>& data) {
    if (block >= dump.blocks.size()) {
        return false;
    }
    dump.blocks[block].data = data;
    dump.blocks[block].known = true;
    return true;
}

bool MifareClassicModel::markBlockUnknown(MifareDump& dump, uint16_t block) {
    if (block >= dump.blocks.size()) {
        return false;
    }
    // Keep the zeroed payload but clear the flag, so a block we never read can
    // never be mistaken for a block that happened to read as zeros.
    dump.blocks[block].known = false;
    return true;
}

bool MifareClassicModel::setSectorKeyA(MifareDump& dump,
                                       uint8_t sector,
                                       const std::array<uint8_t, kMifareKeySize>& key) {
    if (sector >= dump.sectors.size()) {
        return false;
    }
    MifareSectorKeys& entry = dump.sectors[sector];
    // A sector counts as recovered once EITHER key authenticates; counting it
    // per key would report "32 of 16 sectors", which is exactly the kind of
    // inflated progress number this module exists to prevent.
    if (!entry.keyARecovered && !entry.keyBRecovered) {
        ++dump.recoveredSectors;
    }
    entry.keyA = key;
    entry.keyARecovered = true;
    return true;
}

bool MifareClassicModel::setSectorKeyB(MifareDump& dump,
                                       uint8_t sector,
                                       const std::array<uint8_t, kMifareKeySize>& key) {
    if (sector >= dump.sectors.size()) {
        return false;
    }
    MifareSectorKeys& entry = dump.sectors[sector];
    if (!entry.keyARecovered && !entry.keyBRecovered) {
        ++dump.recoveredSectors;
    }
    entry.keyB = key;
    entry.keyBRecovered = true;
    return true;
}

uint16_t MifareClassicModel::unknownBlockCount(const MifareDump& dump) {
    uint16_t unknown = 0;
    for (const MifareDumpBlock& block : dump.blocks) {
        if (!block.known) {
            ++unknown;
        }
    }
    return unknown;
}

uint32_t MifareClassicModel::uid32(const MifareDump& dump) {
    if (dump.uidLength < 4) {
        return 0;
    }
    return (static_cast<uint32_t>(dump.uid[3]) << 24) |
           (static_cast<uint32_t>(dump.uid[2]) << 16) |
           (static_cast<uint32_t>(dump.uid[1]) << 8) |
           static_cast<uint32_t>(dump.uid[0]);
}

std::string MifareClassicModel::recoverySummary(const MifareDump& dump) {
    // Never say "complete" unless the counter says so, and always show the
    // denominator: "16 of 16" and "12 of 16" must look different.
    char buffer[96];
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%u of %u sectors recovered",
                  static_cast<unsigned>(dump.recoveredSectors),
                  static_cast<unsigned>(dump.totalSectors));
    std::string summary(buffer);
    const uint16_t unknown = unknownBlockCount(dump);
    if (unknown != 0) {
        char tail[48];
        std::snprintf(tail, sizeof(tail), ", %u block(s) unread", static_cast<unsigned>(unknown));
        summary += tail;
    }
    return summary;
}
