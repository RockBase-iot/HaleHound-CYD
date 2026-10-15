#include "features/rfid/Iso14443aFraming.h"

#include <cstring>

#include "drivers/IRfidTransport.h"  // rfidAppendCrcA

// ---------------------------------------------------------------------------
// ISO14443-3
// ---------------------------------------------------------------------------

uint8_t Iso14443aFraming::bytesPerCascadeLevel(uint8_t level) {
    switch (level) {
        case 1:
        case 2:
            return 4;
        case 3:
            // The third level only ever carries the last two bytes of a
            // 10-byte UID; the other two slots stay unused.
            return 2;
        default:
            return 0;
    }
}

uint8_t Iso14443aFraming::cascadeLevelCount(uint8_t uidLength) {
    switch (uidLength) {
        case 4:
            return 1;
        case 7:
            return 2;
        case 10:
            return 3;
        default:
            return 0;
    }
}

uint8_t Iso14443aFraming::cascadeCommand(uint8_t level) {
    switch (level) {
        case 1:
            return kIso14443aCascadeLevel1;
        case 2:
            return kIso14443aCascadeLevel2;
        case 3:
            return kIso14443aCascadeLevel3;
        default:
            return 0;
    }
}

uint8_t Iso14443aFraming::blockCheckCharacter(const uint8_t* fourBytes) {
    if (fourBytes == nullptr) {
        return 0;
    }
    return static_cast<uint8_t>(fourBytes[0] ^ fourBytes[1] ^ fourBytes[2] ^
                                fourBytes[3]);
}

uint16_t Iso14443aFraming::buildReqA(uint8_t* out) {
    if (out == nullptr) {
        return 0;
    }
    out[0] = kIso14443aReqA;
    return 1;
}

uint16_t Iso14443aFraming::buildWupA(uint8_t* out) {
    if (out == nullptr) {
        return 0;
    }
    out[0] = kIso14443aWupA;
    return 1;
}

uint16_t Iso14443aFraming::buildSelect(uint8_t level,
                                       const uint8_t* uid,
                                       uint8_t uidLength,
                                       uint8_t* out) {
    if (uid == nullptr || out == nullptr) {
        return 0;
    }
    const uint8_t levels = cascadeLevelCount(uidLength);
    if (levels == 0U || level == 0U || level > levels) {
        return 0;
    }

    // Level L starts at UID byte (L-1)*3. Non-final levels emit the Cascade Tag
    // in the first slot followed by three UID bytes; the final level emits four
    // UID bytes and no tag. That is what makes 4-, 7- and 10-byte UIDs share one
    // frame shape: three UID bytes per non-final level, four on the last.
    const uint8_t offset = static_cast<uint8_t>((level - 1U) * 3U);
    const uint8_t consumed = (level < levels) ? 3U : 4U;
    if (static_cast<uint16_t>(offset) + consumed > static_cast<uint16_t>(uidLength)) {
        return 0;
    }

    uint8_t block[4];
    if (level < levels) {
        // Non-final level: the Cascade Tag occupies the first slot, so the
        // level carries the THREE UID bytes at offset..offset+2.
        block[0] = kIso14443aCascadeTag;
        block[1] = uid[offset];
        block[2] = uid[offset + 1U];
        block[3] = uid[offset + 2U];
    } else {
        // Final level: no cascade tag, four real UID bytes.
        block[0] = uid[offset];
        block[1] = uid[offset + 1U];
        block[2] = uid[offset + 2U];
        block[3] = uid[offset + 3U];
    }

    out[0] = cascadeCommand(level);
    out[1] = kIso14443aNvbSelect;
    out[2] = block[0];
    out[3] = block[1];
    out[4] = block[2];
    out[5] = block[3];
    out[6] = blockCheckCharacter(block);
    rfidAppendCrcA(out, 7);
    return 9;
}

// ---------------------------------------------------------------------------
// MIFARE Classic
// ---------------------------------------------------------------------------

uint16_t Iso14443aFraming::buildAuthenticate(MifareKeySlot slot,
                                             uint8_t block,
                                             const uint8_t* key,
                                             uint8_t* out) {
    if (key == nullptr || out == nullptr) {
        return 0;
    }
    out[0] = (slot == MifareKeySlot::KeyA) ? kMifareCmdAuthenticateA
                                           : kMifareCmdAuthenticateB;
    out[1] = block;
    std::memcpy(out + 2, key, kMifareKeySize);
    return 8;
}

uint16_t Iso14443aFraming::buildReadBlock(uint8_t block, uint8_t* out) {
    if (out == nullptr) {
        return 0;
    }
    out[0] = kMifareCmdRead;
    out[1] = block;
    return 2;
}

uint16_t Iso14443aFraming::buildWriteBlock(uint8_t block,
                                          const uint8_t* data,
                                          uint8_t* out) {
    if (data == nullptr || out == nullptr) {
        return 0;
    }
    out[0] = kMifareCmdWrite;
    out[1] = block;
    std::memcpy(out + 2, data, kMifareBlockSize);
    return 18;
}

// ---------------------------------------------------------------------------
// NTAG / Ultralight
// ---------------------------------------------------------------------------

uint16_t Iso14443aFraming::buildNtagReadPage(uint8_t page, uint8_t* out) {
    if (out == nullptr) {
        return 0;
    }
    out[0] = kNtagCmdRead;
    out[1] = page;
    return 2;
}

uint16_t Iso14443aFraming::buildNtagWritePage(uint8_t page,
                                              const uint8_t* data4,
                                              uint8_t* out) {
    if (data4 == nullptr || out == nullptr) {
        return 0;
    }
    out[0] = kNtagCmdWrite;
    out[1] = page;
    std::memcpy(out + 2, data4, 4);
    return 6;
}

uint16_t Iso14443aFraming::buildNtagGetVersion(uint8_t* out) {
    if (out == nullptr) {
        return 0;
    }
    out[0] = kNtagCmdGetVersion;
    return 1;
}

uint16_t Iso14443aFraming::buildNtagPwdAuth(const uint8_t* password4, uint8_t* out) {
    if (password4 == nullptr || out == nullptr) {
        return 0;
    }
    out[0] = kNtagCmdPwdAuth;
    std::memcpy(out + 1, password4, 4);
    return 5;
}

// ---------------------------------------------------------------------------
// Response helpers
// ---------------------------------------------------------------------------

bool Iso14443aFraming::isMifareAck(const uint8_t* rx, uint16_t rxLength) {
    // Every MIFARE command answers with a 4-bit code. Where the reader puts
    // those four bits depends on the transport (the PN532 leaves them in the
    // low nibble of the single response byte), so only the low nibble is
    // compared -- the high nibble is the card's own response byte for the
    // commands that carry one.
    return rx != nullptr && rxLength >= 1U &&
           (rx[0] & 0x0FU) == (kMifareAck & 0x0FU);
}

bool Iso14443aFraming::isMifareNak(const uint8_t* rx, uint16_t rxLength) {
    return rx != nullptr && rxLength >= 1U &&
           (rx[0] & 0x0FU) == (kMifareNak & 0x0FU);
}

uint8_t Iso14443aFraming::uidLengthFromAtqa(uint16_t atqa) {
    // See the header: the UID-size field is b8/b7, i.e. bits 7/6 of the first
    // ATQA byte. MIFARE Ultralight's 00 44 is the case that catches a wrong
    // reading -- the high byte is zero, yet the UID is 7 bytes long.
    switch ((atqa >> 6) & 0x03U) {
        case 0x00:
            return 4;
        case 0x01:
            return 7;
        case 0x02:
            return 10;
        default:
            return 0;  // RFU: no card may use this
    }
}

Iso14443aCardType Iso14443aFraming::classifyCard(uint16_t atqa, uint8_t sak) {
    switch (sak) {
        case 0x00:
            // Ultralight, Ultralight C and the whole NTAG21x family are
            // indistinguishable here: all answer ATQA 0044 / SAK 00. Only
            // GET_VERSION separates them, so this row stays deliberately
            // coarse instead of guessing a capacity.
            return ((atqa & 0x00FFU) == 0x0044U) ? Iso14443aCardType::UltralightOrNtag
                                                 : Iso14443aCardType::Unknown;
        case 0x08:
            return Iso14443aCardType::MifareClassic1K;
        case 0x18:
            return Iso14443aCardType::MifareClassic4K;
        case 0x09:
            return Iso14443aCardType::MifareMini;
        case 0x10:
            return Iso14443aCardType::PlusSl1_2K;
        case 0x11:
            return Iso14443aCardType::PlusSl1_4K;
        case 0x20:
            return Iso14443aCardType::IsoDep;
        case 0x28:
            return Iso14443aCardType::SmartMxClassic1K;
        case 0x38:
            return Iso14443aCardType::SmartMxClassic4K;
        case 0x88:
            return Iso14443aCardType::InfineonClassic1K;
        case 0x01:
            return Iso14443aCardType::Tnp3xxx;
        default:
            // 0x04 (UID not complete) belongs here too: a SAK returned after a
            // finished activation must never say that, so seeing it means the
            // activation did not complete and nothing about the card is known.
            return Iso14443aCardType::Unknown;
    }
}

const char* Iso14443aFraming::cardTypeLabel(Iso14443aCardType type) {
    switch (type) {
        case Iso14443aCardType::MifareClassic1K:
            return "MIFARE Classic 1K";
        case Iso14443aCardType::MifareClassic4K:
            return "MIFARE Classic 4K";
        case Iso14443aCardType::MifareMini:
            return "MIFARE Mini";
        case Iso14443aCardType::UltralightOrNtag:
            return "Ultralight / NTAG";
        case Iso14443aCardType::IsoDep:
            return "ISO14443-4 (Type 4)";
        case Iso14443aCardType::SmartMxClassic1K:
            return "SmartMX (Classic 1K)";
        case Iso14443aCardType::SmartMxClassic4K:
            return "SmartMX (Classic 4K)";
        case Iso14443aCardType::PlusSl1_2K:
            return "MIFARE Plus SL1 2K";
        case Iso14443aCardType::PlusSl1_4K:
            return "MIFARE Plus SL1 4K";
        case Iso14443aCardType::Tnp3xxx:
            return "TNP3xxx";
        case Iso14443aCardType::InfineonClassic1K:
            return "Infineon Classic 1K";
        case Iso14443aCardType::Unknown:
        default:
            return "unknown card";
    }
}

bool Iso14443aFraming::isMifareClassic(Iso14443aCardType type) {
    switch (type) {
        case Iso14443aCardType::MifareClassic1K:
        case Iso14443aCardType::MifareClassic4K:
        case Iso14443aCardType::MifareMini:
        case Iso14443aCardType::SmartMxClassic1K:
        case Iso14443aCardType::SmartMxClassic4K:
        case Iso14443aCardType::InfineonClassic1K:
            return true;
        default:
            // MIFARE Plus in SL1 mode speaks the Classic protocol too, but it
            // is listed separately because the operator has to know a Plus card
            // can be upgraded mid-session; the dump screen treats it as Classic
            // only after the card itself answers a Classic command.
            return false;
    }
}

bool Iso14443aFraming::isNtagFamily(Iso14443aCardType type) {
    return type == Iso14443aCardType::UltralightOrNtag;
}

bool parseNtagVersion(const uint8_t* rx, uint16_t rxLength, NtagVersionFrame& out) {
    out = NtagVersionFrame{};
    if (rx == nullptr || rxLength < 8U) {
        return false;
    }
    if (rx[0] != 0x00U) {
        // The datasheet fixes byte 0 at 00h. Anything else means this is not a
        // GET_VERSION reply, and mapping it onto an NTAG geometry would invent
        // a capacity the card never claimed.
        return false;
    }
    out.header = rx[0];
    out.vendorId = rx[1];
    out.productType = rx[2];
    out.productSubtype = rx[3];
    out.majorVersion = rx[4];
    out.minorVersion = rx[5];
    out.storageSize = rx[6];
    out.protocolType = rx[7];
    out.valid = true;
    return true;
}
