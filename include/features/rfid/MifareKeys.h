#pragma once

// MIFARE Classic key material.
//
// Two things live here: the published default-key table used by Key Check, and
// the handful of keys the firmware needs by *identity* rather than by position
// (the transport key, the NFC Forum MAD key, and the NDEF data-sector key).
//
// Provenance of the table is deliberately explicit -- see
// docs/protocol-evidence/mifare-classic.md §2. The short version: the upstream
// repository ships no source, so its "29 public keys" claim cannot be audited.
// This table therefore takes the FIRST 29 entries, verbatim and in file order,
// of the public Proxmark3 dictionary `mfc_default_keys.dic` (RfidResearchGroup).
// Taking "the first N in file order" is the only reproduction rule that needs no
// invented ordering. Each entry carries the dictionary's own comment in `note`.
//
// To swap in a different list: replace kKeyTable, keep the count at 29 (the
// static_assert below enforces it), and update the evidence doc. KeyCheckModel
// needs no change -- it iterates the table, it does not know the values.

#include <array>
#include <cstdint>
#include <string>

constexpr uint8_t kMifareKeySize = 6;
constexpr uint16_t kMifareKeyTableCount = 29;

enum class MifareKeySlot : uint8_t {
    KeyA = 0,
    KeyB = 1,
};

struct MifareKey {
    std::array<uint8_t, kMifareKeySize> bytes{};
    // Verbatim comment from the source dictionary. Never empty for a sourced
    // key; "unattributed" would mean somebody added a key without provenance.
    const char* note = "";
};

class MifareKeyTable {
public:
    static constexpr uint16_t kNotFound = 0xFFFF;

    static constexpr uint16_t count() { return kMifareKeyTableCount; }
    static const MifareKey& at(uint16_t index);

    // "FFFFFFFFFFFF" -- the conventional 12-hex-digit form used by .nfc / .eml
    // headers and by every MIFARE tool, so dumps stay diffable against them.
    static std::string toString(const std::array<uint8_t, kMifareKeySize>& key);
    // Accepts upper or lower case, with or without separators (`FF FF FF FF FF FF`,
    // `ff:ff:ff:ff:ff:ff`, `ffffffffffff`). Rejects anything that is not exactly
    // six bytes -- a short key is a bug upstream, not a key we should pad.
    static bool parse(const std::string& text, std::array<uint8_t, kMifareKeySize>& out);

    static bool equal(const std::array<uint8_t, kMifareKeySize>& a,
                      const std::array<uint8_t, kMifareKeySize>& b);
    static uint16_t indexOf(const std::array<uint8_t, kMifareKeySize>& key);

    static bool isTransportKey(const std::array<uint8_t, kMifareKeySize>& key);
    static bool isAllZero(const std::array<uint8_t, kMifareKeySize>& key);
    static bool isAllOnes(const std::array<uint8_t, kMifareKeySize>& key);

    // Keys the firmware references by meaning. These are entries of the table
    // above, exposed by name so call sites do not hardcode a table index that
    // would silently change if the list is reordered.
    static const std::array<uint8_t, kMifareKeySize>& transportKey();   // FF FF FF FF FF FF
    static const std::array<uint8_t, kMifareKeySize>& madKeyA();        // A0 A1 A2 A3 A4 A5
    static const std::array<uint8_t, kMifareKeySize>& ndefSectorKeyA(); // D3 F7 D3 F7 D3 F7
};
