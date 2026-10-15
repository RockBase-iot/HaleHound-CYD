#pragma once

// Card dump interchange formats.
//
// Upstream v4.0 stores MIFARE Classic and NTAG dumps on the SD card as a real
// Flipper Zero ``.nfc`` file plus a Proxmark ``.eml`` file, and loads both back
// in from the saved-dump browser with no card on the antenna. This module owns
// the text format side of that: build the file body, parse it back.
//
// Only the subset of each format this firmware produces is parsed. Unknown
// keys in an ``.nfc`` header are preserved in ``unknownKeys`` so a dump that
// travelled through a newer Flipper firmware is not silently degraded on
// re-save.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class IFeature;

constexpr size_t kCardDumpBlockSize = 16;

enum class CardDumpKind : uint8_t {
    MifareClassic,
    Ntag21x,
    Ultralight,
    Unknown,
};

struct CardDumpBlock {
    std::array<uint8_t, kCardDumpBlockSize> data{};
    bool known = true;  // false means the reader could not authenticate this block
};

struct CardDump {
    CardDumpKind kind = CardDumpKind::Unknown;
    std::string deviceType;  // verbatim "Device type:" line, e.g. "Mifare Classic"
    std::array<uint8_t, 10> uid{};
    uint8_t uidLength = 0;
    uint16_t atqa = 0;
    uint8_t sak = 0;
    std::string mifareType;  // "Mini", "1K" or "4K"
    std::vector<CardDumpBlock> blocks;
    std::vector<std::string> unknownKeys;
};

struct CardDumpResult {
    bool valid = false;
    const char* detail = "no dump";
    CardDump dump;
};

struct CardDumpText {
    bool valid = false;
    const char* detail = "no text";
    std::string text;
};

class CardDumpFormat {
public:
    // ---- serialisation ----------------------------------------------------
    static CardDumpText toFlipperNfc(const CardDump& dump);
    static CardDumpText toProxmarkEml(const CardDump& dump);

    // ---- parsing ----------------------------------------------------------
    static CardDumpResult fromFlipperNfc(const std::string& text);
    static CardDumpResult fromProxmarkEml(const std::string& text);

    // Picks the parser from the file extension, mirroring the saved-dump
    // browser so both formats open through one entry point.
    static CardDumpResult parseByExtension(const std::string& fileName, const std::string& text);

    // ---- helpers ----------------------------------------------------------
    // Number of 16-byte blocks a MIFARE Classic size string implies.
    static uint16_t blocksForMifareType(const std::string& mifareType);
    static const char* kindLabel(CardDumpKind kind);
    static std::string formatUid(const CardDump& dump);
};

