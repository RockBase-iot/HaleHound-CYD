#pragma once

// ISO/IEC 14443-3 activation frames, and the MIFARE / NTAG command frames that
// ride on them.
//
// These are card-side byte sequences: the same bytes leave the host whether the
// reader underneath is a PN532 or an ST25R3916. They therefore belong above the
// transport, not inside it, and they are pure functions -- no radio, no
// allocation -- so the host suites can pin them.
//
// Provenance: NXP MF1S50YYX_V1 (MIFARE Classic), NXP MF0ICU1/2 (Ultralight),
// NXP NTAG213/215/216, and ISO/IEC 14443-3:2018 clause 6. See
// docs/protocol-evidence/mifare-classic.md and docs/protocol-evidence/pn532.md.

#include <cstdint>

#include "features/rfid/MifareClassicModel.h"  // kMifareBlockSize
#include "features/rfid/MifareKeys.h"          // kMifareKeySize, MifareKeySlot
#include "drivers/IRfidTransport.h"            // rfidCrcA / rfidAppendCrcA / rfidCheckCrcA

// ---- ISO14443-3 ----------------------------------------------------------
// REQA / WUPA are 7-bit short frames and carry no CRC.
constexpr uint8_t kIso14443aReqA = 0x26;
constexpr uint8_t kIso14443aWupA = 0x52;
// Anticollision / SELECT are the same byte per cascade level.
constexpr uint8_t kIso14443aCascadeLevel1 = 0x93;
constexpr uint8_t kIso14443aCascadeLevel2 = 0x95;
constexpr uint8_t kIso14443aCascadeLevel3 = 0x97;
// Cascade Tag. Substituted for the first UID byte on every level except the
// last, so 7- and 10-byte UIDs ride through the same 4-byte-wide frame.
constexpr uint8_t kIso14443aCascadeTag = 0x88;
// NVB values: 0x20 asks for the 4-byte anticollision response, 0x70 selects.
constexpr uint8_t kIso14443aNvbAnticollision = 0x20;
constexpr uint8_t kIso14443aNvbSelect = 0x70;
// The CID-less PCB of an I-Block with chaining off (ISO14443-4 §7.1.1).
constexpr uint8_t kIsoDepPcbIBlock = 0x02;

// ---- MIFARE Classic (MF1S50YYX_V1 §10) -----------------------------------
constexpr uint8_t kMifareCmdAuthenticateA = 0x60;
constexpr uint8_t kMifareCmdAuthenticateB = 0x61;
constexpr uint8_t kMifareCmdRead = 0x30;      // + block  -> 16 bytes
constexpr uint8_t kMifareCmdWrite = 0xA0;     // + block + 16 bytes -> ACK
constexpr uint8_t kMifareCmdDecrement = 0xC0; // + block + 4 bytes
constexpr uint8_t kMifareCmdIncrement = 0xC1;
constexpr uint8_t kMifareCmdRestore = 0xC2;
constexpr uint8_t kMifareCmdTransfer = 0xB0;
constexpr uint8_t kMifareCmdHalt = 0x50;

// Every MIFARE response is a 4-bit nibble in the low half of one byte.
constexpr uint8_t kMifareAck = 0x0A;
constexpr uint8_t kMifareNak = 0x00;

// ---- NTAG / MIFARE Ultralight (MF0ICU1, NTAG21x) -------------------------
constexpr uint8_t kNtagCmdRead = 0x30;    // + page -> 16 bytes (4 pages)
constexpr uint8_t kNtagCmdWrite = 0xA2;   // + page + 4 bytes -> ACK
constexpr uint8_t kNtagCmdGetVersion = 0x60;  // NTAG21x only -> 8 bytes
constexpr uint8_t kNtagCmdReadCnt = 0x39;
constexpr uint8_t kNtagCmdPwdAuth = 0x1B;     // + 4 bytes
constexpr uint8_t kNtagCmdFastRead = 0x3A;    // + start + end

// ---- card classification -------------------------------------------------
enum class Iso14443aCardType : uint8_t {
    Unknown,
    MifareClassic1K,
    MifareClassic4K,
    MifareMini,
    UltralightOrNtag,
    IsoDep,          // ISO14443-4: DESFire, MIFARE Plus SL3, Type 4 tags
    SmartMxClassic1K,
    SmartMxClassic4K,
    PlusSl1_2K,
    PlusSl1_4K,
    Tnp3xxx,
    InfineonClassic1K,
};

class Iso14443aFraming {
public:
    // ---- ISO14443-3 framing ----------------------------------------------
    // Bytes a UID occupies on the wire at a given cascade level: 4 for levels
    // 1 and 2, 2 for level 3. Returns 0 when `level` is out of range.
    static uint8_t bytesPerCascadeLevel(uint8_t level);
    // How many cascade levels a UID of this length needs: 1 (4 bytes), 2 (7),
    // 3 (10). Returns 0 for a length no ISO14443-3 card can have.
    static uint8_t cascadeLevelCount(uint8_t uidLength);
    static uint8_t cascadeCommand(uint8_t level);  // 1 -> 0x93, 2 -> 0x95, ...

    // BCC is the XOR of the four bytes it follows. Returns 0 for a level
    // shorter than four bytes.
    static uint8_t blockCheckCharacter(const uint8_t* fourBytes);

    // REQA/WUPA are single bytes with no CRC. `out` needs 1 byte.
    static uint16_t buildReqA(uint8_t* out);
    static uint16_t buildWupA(uint8_t* out);

    // SELECT for one cascade level: cmd, 0x70, <4 uid bytes>, BCC, CRC.
    // `out` needs 9 bytes. Returns the frame length, or 0 when the UID is too
    // short for the requested level.
    static uint16_t buildSelect(uint8_t level,
                                const uint8_t* uid,
                                uint8_t uidLength,
                                uint8_t* out);

    // ---- MIFARE Classic commands -----------------------------------------
    // 0x60/0x61, block, key[6]. `out` needs 8 bytes (no CRC: the frames are
    // assembled by the reader silicon, which owns CRC and the Crypto1
    // handshake on this transport).
    static uint16_t buildAuthenticate(MifareKeySlot slot,
                                      uint8_t block,
                                      const uint8_t* key,
                                      uint8_t* out);
    // 0x30, block. `out` needs 2.
    static uint16_t buildReadBlock(uint8_t block, uint8_t* out);
    // 0xA0, block, 16 data bytes. `out` needs 18.
    static uint16_t buildWriteBlock(uint8_t block, const uint8_t* data, uint8_t* out);

    // ---- NTAG / Ultralight commands --------------------------------------
    // 0x30, page. `out` needs 2. Returns four pages (16 bytes) on the wire.
    static uint16_t buildNtagReadPage(uint8_t page, uint8_t* out);
    // 0xA2, page, 4 data bytes. `out` needs 6.
    static uint16_t buildNtagWritePage(uint8_t page, const uint8_t* data4, uint8_t* out);
    // 0x60. `out` needs 1. NTAG21x answers with an 8-byte version frame.
    static uint16_t buildNtagGetVersion(uint8_t* out);
    // 0x1B, password[4]. `out` needs 5. Unlocks pages from AUTH0 onwards.
    static uint16_t buildNtagPwdAuth(const uint8_t* password4, uint8_t* out);

    // ---- response helpers -------------------------------------------------
    // MIFARE answers every command with a 4-bit code in the low nibble; the
    // high nibble carries the card's own response byte for some commands.
    static bool isMifareAck(const uint8_t* rx, uint16_t rxLength);
    static bool isMifareNak(const uint8_t* rx, uint16_t rxLength);

    // SAK is the authoritative "what kind of card is this" byte; ATQA only
    // narrows the UID length and the 106 kbps framing. NXP AN10833 Table 6 is
    // the source for the SAK rows, and it is the *last* SAK of the activation
    // that counts: with a double- or triple-size UID only the final cascade
    // level's SAK identifies the chip. Both halves are used so an unknown
    // pairing reports Unknown rather than a confident guess.
    static Iso14443aCardType classifyCard(uint16_t atqa, uint8_t sak);
    static const char* cardTypeLabel(Iso14443aCardType type);
    // ATQA codes the UID size in bits b8 and b7, which are bits 7 and 6 of the
    // *first* ATQA byte -- not of the 16-bit value, and not in the high byte.
    // AN10833 Table 5 is explicit: MIFARE Ultralight answers 00 44, and the
    // "1 0" at b8/b7 is what makes it a double-size (7-byte) UID, even though
    // the high byte is zero. `atqa` is little-endian, so the field is
    // (atqa >> 6) & 0x03. 0b00 -> 4 bytes, 0b01 -> 7, 0b10 -> 10, 0b11 -> RFU.
    static uint8_t uidLengthFromAtqa(uint16_t atqa);

    // Whether a card type carries MIFARE Classic sectors at all. Used by the
    // dump screen to refuse a dump it cannot represent.
    static bool isMifareClassic(Iso14443aCardType type);
    static bool isNtagFamily(Iso14443aCardType type);
};

// The 8-byte GET_VERSION reply, verbatim from the NXP NTAG213/215/216
// datasheet Table 28.
struct NtagVersionFrame {
    bool valid = false;
    uint8_t header = 0;       // always 0x00
    uint8_t vendorId = 0;     // 0x04 = NXP
    uint8_t productType = 0;  // 0x04 = NTAG, 0x03 = MIFARE Ultralight
    uint8_t productSubtype = 0;
    uint8_t majorVersion = 0;
    uint8_t minorVersion = 0;
    uint8_t storageSize = 0;  // 0x0F / 0x11 / 0x13 for NTAG213/215/216
    uint8_t protocolType = 0; // 0x03 = ISO/IEC 14443-3 compliant
};

// Parses the GET_VERSION reply. Returns false (and leaves `valid` false) when
// the frame is shorter than 8 bytes or the leading header byte is not 0x00, so
// a part that does not implement GET_VERSION reports "unsupported" rather than
// being silently mapped onto a small NTAG.
bool parseNtagVersion(const uint8_t* rx, uint16_t rxLength, NtagVersionFrame& out);
