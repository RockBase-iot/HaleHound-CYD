#include "features/rfid/IclassPacs.h"

#include <cstdio>

namespace {

// HID H10301, 26 bits, widely published and verifiable against any 26-bit
// credential: leading even parity, 8-bit facility code, 16-bit card number,
// trailing odd parity.
constexpr PacsFormatSpec kHid26 = {
    PacsFormatId::Hid26,
    "HID 26-bit (H10301)",
    26,
    2,
    {
        {PacsFieldId::FacilityCode, "Facility", 17, 8},
        {PacsFieldId::CardNumber, "Card", 1, 16},
        {PacsFieldId::IssueCode, "Issue", 0, 0},
    },
    25,
    0,
    13,
    24,  // even parity over the 12 bits after the leading parity
    1,
    12,  // odd parity over the 12 bits before the trailing parity
    true,
};

// HID Corporate 1000, 35 bits: 12-bit corporate code and 20-bit card number.
// The field ranges follow the published layout; the parity convention is not
// confirmed against a vendor document, so parity is extracted but not asserted.
constexpr PacsFormatSpec kCorporate1000 = {
    PacsFormatId::Corporate1000,
    "Corporate 1000 (35-bit)",
    35,
    2,
    {
        {PacsFieldId::FacilityCode, "Corporate", 22, 12},
        {PacsFieldId::CardNumber, "Card", 2, 20},
        {PacsFieldId::IssueCode, "Issue", 0, 0},
    },
    34,
    1,
    22,
    33,
    2,
    21,
    false,
};

// HID 48-bit: 22-bit site/facility code and 24-bit card number. Same caveat as
// Corporate 1000: layout extracted, parity not asserted.
constexpr PacsFormatSpec kHid48 = {
    PacsFormatId::Hid48,
    "HID 48-bit",
    48,
    2,
    {
        {PacsFieldId::FacilityCode, "Site", 25, 22},
        {PacsFieldId::CardNumber, "Card", 1, 24},
        {PacsFieldId::IssueCode, "Issue", 0, 0},
    },
    47,
    0,
    25,
    46,
    1,
    24,
    false,
};

constexpr const PacsFormatSpec* const kFormats[] = {&kHid26, &kCorporate1000, &kHid48};

uint64_t rangeMask(uint8_t bits) {
    if (bits == 0) {
        return 0;
    }
    if (bits >= 64) {
        return ~static_cast<uint64_t>(0);
    }
    return (static_cast<uint64_t>(1) << bits) - 1;
}

}  // namespace

const PacsFormatSpec* IclassPacs::format(PacsFormatId id) {
    for (const PacsFormatSpec* candidate : kFormats) {
        if (candidate->id == id) {
            return candidate;
        }
    }
    return NULL;
}

const PacsFormatSpec* IclassPacs::formatForBitCount(uint8_t bitCount) {
    for (const PacsFormatSpec* candidate : kFormats) {
        if (candidate->bitCount == bitCount) {
            return candidate;
        }
    }
    return NULL;
}

bool IclassPacs::parseBitString(const std::string& bits, uint64_t& raw, uint8_t& bitCount) {
    raw = 0;
    bitCount = 0;
    for (size_t index = 0; index < bits.size(); ++index) {
        const char character = bits[index];
        if (character == ' ' || character == '_' || character == '\t') {
            continue;
        }
        if (character != '0' && character != '1') {
            return false;
        }
        if (bitCount >= 64) {
            return false;
        }
        raw = (raw << 1) | (character == '1' ? 1ULL : 0ULL);
        ++bitCount;
    }
    return bitCount > 0;
}

std::string IclassPacs::toBitString(uint64_t raw, uint8_t bitCount) {
    std::string text;
    if (bitCount == 0) {
        return text;
    }
    const uint8_t capped = bitCount > 64 ? 64 : bitCount;
    text.reserve(capped + capped / 8);
    for (int position = capped - 1; position >= 0; --position) {
        text.push_back(((raw >> position) & 1ULL) != 0 ? '1' : '0');
        if (position != 0 && (position % 4) == 0) {
            text.push_back(' ');
        }
    }
    return text;
}

bool IclassPacs::evenParity(uint64_t raw, uint8_t from, uint8_t to) {
    if (to < from || to >= 64) {
        return false;
    }
    uint8_t ones = 0;
    for (uint8_t bit = from; bit <= to; ++bit) {
        ones = static_cast<uint8_t>(ones + ((raw >> bit) & 1ULL));
    }
    return (ones % 2) == 0;
}

bool IclassPacs::oddParity(uint64_t raw, uint8_t from, uint8_t to) {
    if (to < from || to >= 64) {
        return false;
    }
    return !evenParity(raw, from, to);
}

PacsDecoded IclassPacs::decode(uint64_t raw, uint8_t bitCount) {
    PacsDecoded result;
    result.raw = raw;
    result.bitString = toBitString(raw, bitCount);

    const PacsFormatSpec* spec = formatForBitCount(bitCount);
    if (spec == NULL) {
        result.detail = "unsupported framing";
        return result;
    }
    result.format = spec->id;

    for (uint8_t index = 0; index < spec->fieldCount; ++index) {
        const PacsFieldSpec& field = spec->fields[index];
        if (field.bits == 0) {
            continue;
        }
        const uint64_t value = (raw >> field.shift) & rangeMask(field.bits);
        switch (field.id) {
            case PacsFieldId::FacilityCode:
                result.facilityCode = static_cast<uint32_t>(value);
                break;
            case PacsFieldId::CardNumber:
                result.cardNumber = value;
                break;
            case PacsFieldId::IssueCode:
                result.issueCode = static_cast<uint32_t>(value);
                break;
        }
    }

    if (spec->parityConfirmed) {
        const bool leading =
            ((raw >> spec->leadingParityBit) & 1ULL) != 0;
        const bool trailing =
            ((raw >> spec->trailingParityBit) & 1ULL) != 0;
        // The leading bit is an even-parity bit and the trailing bit an
        // odd-parity bit, each counted together with the group it covers. An
        // even-parity bit is therefore 1 when its group has an odd number of
        // ones, and an odd-parity bit is 1 when its group has an even count.
        result.leadingParityOk =
            leading == oddParity(raw, spec->leadingParityFrom, spec->leadingParityTo);
        result.trailingParityOk =
            trailing == evenParity(raw, spec->trailingParityFrom, spec->trailingParityTo);
        result.parityChecked = true;
    }

    result.valid = true;
    result.detail = spec->parityConfirmed ? "decoded" : "decoded, parity not confirmed";
    return result;
}

PacsDecoded IclassPacs::decodeBitString(const std::string& bits) {
    uint64_t raw = 0;
    uint8_t bitCount = 0;
    if (!parseBitString(bits, raw, bitCount)) {
        PacsDecoded result;
        result.detail = "invalid bit string";
        return result;
    }
    return decode(raw, bitCount);
}

std::string IclassPacs::formatCredential(const PacsDecoded& decoded) {
    char buffer[64];
    if (decoded.format == PacsFormatId::Hid26) {
        std::snprintf(buffer, sizeof(buffer), "%u:%u", decoded.facilityCode,
                      static_cast<unsigned>(decoded.cardNumber));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%u:%llu", decoded.facilityCode,
                      static_cast<unsigned long long>(decoded.cardNumber));
    }
    return std::string(buffer);
}

const char* IclassPacs::formatLabel(PacsFormatId id) {
    const PacsFormatSpec* spec = format(id);
    return spec == NULL ? "Unknown" : spec->label;
}
