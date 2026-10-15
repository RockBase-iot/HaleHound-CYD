#include "features/rfid/MifareKeys.h"

#include <cctype>
#include <cstring>

namespace {

// First 29 entries of Proxmark3 `client/dictionaries/mfc_default_keys.dic`,
// verbatim and in file order. See the header and
// docs/protocol-evidence/mifare-classic.md §2 for the reproduction rule.
constexpr MifareKey kKeyTable[kMifareKeyTableCount] = {
    {{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}, "factory transport key"},
    {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, "blank (all-zero) key"},
    {{{0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}}, "NFC Forum MAD key A"},
    {{{0xA5, 0xA4, 0xA3, 0xA2, 0xA1, 0xA0}}, "MAD access key A (reversed)"},
    {{{0x89, 0xEC, 0xA9, 0x7F, 0x8C, 0x2A}}, "MAD access key B"},
    {{{0x5C, 0x8F, 0xF9, 0x99, 0x0D, 0xA2}}, "MF1K EV1 signature block 16 key A"},
    {{{0x75, 0xCC, 0xB5, 0x9C, 0x9B, 0xED}}, "MF1K EV1 signature block 17 key A"},
    {{{0xD0, 0x1A, 0xFE, 0xEB, 0x89, 0x0A}}, "MF1K EV1 signature block 16 key B"},
    {{{0x4B, 0x79, 0x1B, 0xEA, 0x7B, 0xCC}}, "MF1K EV1 signature block 17 key B"},
    {{{0x48, 0x23, 0x74, 0x13, 0x86, 0xAB}}, "MIFARE Plus SE post-1K key A"},
    {{{0x32, 0x65, 0x87, 0xC3, 0xD1, 0x7F}}, "MIFARE Plus SE post-1K key B"},
    {{{0x26, 0x12, 0xC6, 0xDE, 0x84, 0xCA}}, "QL88 block 17 A/B"},
    {{{0x70, 0x7B, 0x11, 0xFC, 0x14, 0x81}}, "QL88 block 17 A/B"},
    {{{0x03, 0xF9, 0x06, 0x76, 0x46, 0xAE}}, "QL88 diversified"},
    {{{0x23, 0x52, 0xC5, 0xB5, 0x6D, 0x85}}, "QL88 diversified"},
    {{{0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}}, "widely published default"},
    {{{0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5}}, "widely published default"},
    {{{0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5}}, "widely published default"},
    {{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}}, "widely published default"},
    {{{0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD}}, "widely published default"},
    {{{0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A}}, "widely published default"},
    {{{0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA}}, "gym / fitness wristband default"},
    {{{0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB}}, "gym / fitness wristband default"},
    {{{0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}}, "key A Wien; NFC Forum NDEF data-sector key A"},
    {{{0x5A, 0x1B, 0x85, 0xFC, 0xE2, 0x0A}}, "key B Wien"},
    {{{0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97}}, "widely published default"},
    {{{0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0F}}, "widely published default"},
    {{{0xA0, 0x47, 0x8C, 0xC3, 0x90, 0x91}}, "widely published default"},
    {{{0x53, 0x3C, 0xB6, 0xC7, 0x23, 0xF6}}, "widely published default"},
};

static_assert(sizeof(kKeyTable) / sizeof(kKeyTable[0]) == kMifareKeyTableCount,
              "key table size must match kMifareKeyTableCount (see docs/protocol-evidence)");

constexpr char kHexDigits[] = "0123456789ABCDEF";

int hexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

}  // namespace

const MifareKey& MifareKeyTable::at(uint16_t index) {
    return kKeyTable[index < kMifareKeyTableCount ? index : 0];
}

std::string MifareKeyTable::toString(const std::array<uint8_t, kMifareKeySize>& key) {
    std::string out;
    out.reserve(kMifareKeySize * 2);
    for (uint8_t byte : key) {
        out.push_back(kHexDigits[(byte >> 4) & 0x0F]);
        out.push_back(kHexDigits[byte & 0x0F]);
    }
    return out;
}

bool MifareKeyTable::parse(const std::string& text, std::array<uint8_t, kMifareKeySize>& out) {
    std::array<uint8_t, kMifareKeySize> parsed{};
    uint8_t byteIndex = 0;
    int high = -1;
    for (char c : text) {
        // Separators are cosmetic; accept the forms every MIFARE tool emits.
        if (c == ' ' || c == ':' || c == '-' || c == '\t' || c == '\r') {
            continue;
        }
        const int nibble = hexNibble(c);
        if (nibble < 0) {
            return false;
        }
        if (high < 0) {
            high = nibble;
            continue;
        }
        if (byteIndex >= kMifareKeySize) {
            // More than six bytes: reject rather than truncate. A caller that
            // silently keeps the first six bytes would authenticate with the
            // wrong key and report a false "no match".
            return false;
        }
        parsed[byteIndex++] = static_cast<uint8_t>((high << 4) | nibble);
        high = -1;
    }
    if (high >= 0 || byteIndex != kMifareKeySize) {
        return false;
    }
    out = parsed;
    return true;
}

bool MifareKeyTable::equal(const std::array<uint8_t, kMifareKeySize>& a,
                           const std::array<uint8_t, kMifareKeySize>& b) {
    return a == b;
}

uint16_t MifareKeyTable::indexOf(const std::array<uint8_t, kMifareKeySize>& key) {
    for (uint16_t index = 0; index < kMifareKeyTableCount; ++index) {
        if (kKeyTable[index].bytes == key) {
            return index;
        }
    }
    return kNotFound;
}

bool MifareKeyTable::isTransportKey(const std::array<uint8_t, kMifareKeySize>& key) {
    return key == transportKey();
}

bool MifareKeyTable::isAllZero(const std::array<uint8_t, kMifareKeySize>& key) {
    for (uint8_t byte : key) {
        if (byte != 0x00) {
            return false;
        }
    }
    return true;
}

bool MifareKeyTable::isAllOnes(const std::array<uint8_t, kMifareKeySize>& key) {
    for (uint8_t byte : key) {
        if (byte != 0xFF) {
            return false;
        }
    }
    return true;
}

const std::array<uint8_t, kMifareKeySize>& MifareKeyTable::transportKey() {
    return kKeyTable[0].bytes;
}

const std::array<uint8_t, kMifareKeySize>& MifareKeyTable::madKeyA() {
    return kKeyTable[2].bytes;
}

const std::array<uint8_t, kMifareKeySize>& MifareKeyTable::ndefSectorKeyA() {
    return kKeyTable[23].bytes;
}
