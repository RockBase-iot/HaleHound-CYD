#pragma once

// iCLASS / HID PACS bit layout decoding.
//
// Upstream v4.0 reads iCLASS cards across AA1 and AA2, performs 3DES on the
// board, pulls the PACS bits out of the decrypted block and decodes the 26, 35
// and 48-bit formats, Corporate 1000 included.
//
// This module owns the bit slicing and parity side of that. It is deliberately
// table-driven: a format is a row of named bit ranges, so the field widths and
// parity positions are auditable in one place instead of being baked into
// branches. Formats whose exact parity convention has not been confirmed
// against a vendor document carry ``parityConfirmed = false``; decoding still
// extracts the fields, but the result reports that parity was not asserted.
// See docs/protocol-evidence/ for the provenance convention this follows.

#include <cstdint>
#include <string>

class IFeature;

enum class PacsFormatId : uint8_t {
    Hid26,
    Corporate1000,
    Hid48,
    Unknown,
};

enum class PacsFieldId : uint8_t {
    FacilityCode,
    CardNumber,
    IssueCode,
};

struct PacsFieldSpec {
    PacsFieldId id;
    const char* label;
    uint8_t shift;  // bit offset of the least significant bit, counted from 0
    uint8_t bits;
};

struct PacsFormatSpec {
    PacsFormatId id;
    const char* label;
    uint8_t bitCount;
    uint8_t fieldCount;
    PacsFieldSpec fields[3];
    // Bit position of the leading even-parity bit and the trailing odd-parity
    // bit, or 0xFF when the format has no parity bit at that end.
    uint8_t leadingParityBit;
    uint8_t trailingParityBit;
    // Which bits each parity covers: [start, end] inclusive, counted from 0.
    uint8_t leadingParityFrom;
    uint8_t leadingParityTo;
    uint8_t trailingParityFrom;
    uint8_t trailingParityTo;
    bool parityConfirmed;
};

struct PacsDecoded {
    bool valid = false;
    const char* detail = "not decoded";
    PacsFormatId format = PacsFormatId::Unknown;
    uint64_t raw = 0;
    uint32_t facilityCode = 0;
    uint64_t cardNumber = 0;
    uint32_t issueCode = 0;
    bool parityChecked = false;
    bool leadingParityOk = false;
    bool trailingParityOk = false;
    std::string bitString;
};

class IclassPacs {
public:
    static const PacsFormatSpec* format(PacsFormatId id);
    // Picks the format whose bit count matches. Returns nullptr when no format
    // of that width is known, which the caller surfaces as "unsupported
    // framing" rather than as a silent card.
    static const PacsFormatSpec* formatForBitCount(uint8_t bitCount);

    // Bit strings are most-significant-bit first, whitespace and underscores
    // ignored. Returns false when the text contains a non-binary character.
    static bool parseBitString(const std::string& bits, uint64_t& raw, uint8_t& bitCount);
    static std::string toBitString(uint64_t raw, uint8_t bitCount);

    static PacsDecoded decode(uint64_t raw, uint8_t bitCount);
    static PacsDecoded decodeBitString(const std::string& bits);

    // Even parity over the inclusive bit range, counted from bit 0.
    static bool evenParity(uint64_t raw, uint8_t from, uint8_t to);
    static bool oddParity(uint64_t raw, uint8_t from, uint8_t to);

    // "facility:card" rendering, zero padded to the field widths.
    static std::string formatCredential(const PacsDecoded& decoded);
    static const char* formatLabel(PacsFormatId id);
};

