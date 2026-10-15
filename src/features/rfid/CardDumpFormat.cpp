#include "features/rfid/CardDumpFormat.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::string hexByte(uint8_t value) {
    char buffer[4];
    std::snprintf(buffer, sizeof(buffer), "%02X", value);
    return std::string(buffer);
}

// Trims leading/trailing ASCII whitespace without touching interior spacing.
std::string trim(const std::string& text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r' ||
                           text[begin] == '\n')) {
        ++begin;
    }
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r' ||
                           text[end - 1] == '\n')) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool startsWith(const std::string& text, const char* prefix) {
    const size_t length = std::strlen(prefix);
    return text.size() >= length && text.compare(0, length, prefix) == 0;
}

// Splits "04 12 34" (or "04:12:34") into bytes. Returns false on a bad token so
// a corrupted dump is rejected instead of half-loaded.
bool parseBytes(const std::string& text, std::vector<uint8_t>& out) {
    out.clear();
    size_t cursor = 0;
    while (cursor < text.size()) {
        while (cursor < text.size() &&
               (text[cursor] == ' ' || text[cursor] == ':' || text[cursor] == '\t')) {
            ++cursor;
        }
        if (cursor >= text.size()) {
            break;
        }
        if (cursor + 2 > text.size()) {
            return false;
        }
        const std::string token = text.substr(cursor, 2);
        char* end = NULL;
        const unsigned long value = std::strtoul(token.c_str(), &end, 16);
        if (end == NULL || *end != '\0') {
            return false;
        }
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        cursor += 2;
    }
    return true;
}

// A block the reader never recovered is written as sixteen '??' placeholders
// (see toFlipperNfc). That is not hex, so it has to be recognised before
// parseBytes -- otherwise the line is unparseable and the block is lost.
bool isUnknownPayload(const std::string& text) {
    bool sawToken = false;
    size_t cursor = 0;
    while (cursor < text.size()) {
        const char c = text[cursor];
        if (c == ' ' || c == '\t' || c == '\r' || c == ':') {
            ++cursor;
            continue;
        }
        if (cursor + 2 > text.size() || text[cursor] != '?' || text[cursor + 1] != '?') {
            return false;
        }
        sawToken = true;
        cursor += 2;
    }
    return sawToken;
}

struct BlockPayload {
    std::vector<uint8_t> bytes;
    bool unknown = false;
};

// "Block 7: AA BB ..." -> block index and payload. Returns false if the line is
// not a block line.
bool parseBlockLine(const std::string& line, size_t& index, BlockPayload& payload) {
    if (!startsWith(line, "Block ")) {
        return false;
    }
    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    const std::string number = trim(line.substr(6, colon - 6));
    index = static_cast<size_t>(std::strtoul(number.c_str(), NULL, 10));
    const std::string payloadText = line.substr(colon + 1);
    if (isUnknownPayload(payloadText)) {
        payload.bytes.clear();
        payload.unknown = true;
        return true;
    }
    std::vector<uint8_t> bytes;
    if (!parseBytes(payloadText, bytes)) {
        return false;
    }
    payload.bytes = bytes;
    return true;
}

}  // namespace

uint16_t CardDumpFormat::blocksForMifareType(const std::string& mifareType) {
    if (mifareType == "Mini") {
        return 20;
    }
    if (mifareType == "4K") {
        return 256;
    }
    if (mifareType == "1K") {
        return 64;
    }
    return 0;
}

const char* CardDumpFormat::kindLabel(CardDumpKind kind) {
    switch (kind) {
        case CardDumpKind::MifareClassic:
            return "MIFARE Classic";
        case CardDumpKind::Ntag21x:
            return "NTAG 21x";
        case CardDumpKind::Ultralight:
            return "Ultralight";
        case CardDumpKind::Unknown:
        default:
            return "Unknown";
    }
}

std::string CardDumpFormat::formatUid(const CardDump& dump) {
    std::string text;
    for (uint8_t index = 0; index < dump.uidLength && index < dump.uid.size(); ++index) {
        if (index != 0) {
            text += " ";
        }
        text += hexByte(dump.uid[index]);
    }
    return text;
}

CardDumpText CardDumpFormat::toFlipperNfc(const CardDump& dump) {
    CardDumpText result;
    if (dump.blocks.empty()) {
        result.detail = "empty dump";
        return result;
    }

    std::string text = "Filetype: Flipper NFC device\nVersion: 4\n";
    text += "Device type: ";
    text += dump.deviceType.empty() ? kindLabel(dump.kind) : dump.deviceType;
    text += "\n";
    text += "UID: " + formatUid(dump) + "\n";

    char header[48];
    std::snprintf(header, sizeof(header), "ATQA: %02X %02X\n", static_cast<uint8_t>(dump.atqa & 0xFF),
                  static_cast<uint8_t>((dump.atqa >> 8) & 0xFF));
    text += header;
    std::snprintf(header, sizeof(header), "SAK: %02X\n", dump.sak);
    text += header;

    if (dump.kind == CardDumpKind::MifareClassic) {
        text += "Mifare Classic type: ";
        text += dump.mifareType.empty() ? "1K" : dump.mifareType;
        text += "\n";
        text += "Data format version: 2\n";
    }

    for (const std::string& key : dump.unknownKeys) {
        text += key;
        text += "\n";
    }

    for (size_t index = 0; index < dump.blocks.size(); ++index) {
        char line[128];
        std::snprintf(line, sizeof(line), "Block %u: ", static_cast<unsigned>(index));
        text += line;
        if (!dump.blocks[index].known) {
            // Flipper uses '??' for a block the reader did not recover.
            text += "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??";
        } else {
            for (size_t byte = 0; byte < kCardDumpBlockSize; ++byte) {
                if (byte != 0) {
                    text += " ";
                }
                text += hexByte(dump.blocks[index].data[byte]);
            }
        }
        text += "\n";
    }

    result.valid = true;
    result.detail = "flipper nfc";
    result.text = text;
    return result;
}

CardDumpText CardDumpFormat::toProxmarkEml(const CardDump& dump) {
    CardDumpText result;
    if (dump.blocks.empty()) {
        result.detail = "empty dump";
        return result;
    }

    std::string text;
    for (const CardDumpBlock& block : dump.blocks) {
        if (!block.known) {
            // A Proxmark .eml cannot express "unknown"; skip the block so a
            // partial read never looks like a fully recovered card.
            continue;
        }
        for (size_t byte = 0; byte < kCardDumpBlockSize; ++byte) {
            if (byte != 0) {
                text += " ";
            }
            text += hexByte(block.data[byte]);
        }
        text += "\n";
    }

    if (text.empty()) {
        result.detail = "no known blocks";
        return result;
    }
    result.valid = true;
    result.detail = "proxmark eml";
    result.text = text;
    return result;
}

CardDumpResult CardDumpFormat::fromFlipperNfc(const std::string& text) {
    CardDumpResult result;
    CardDump& dump = result.dump;
    bool sawFiletype = false;

    size_t cursor = 0;
    while (cursor < text.size()) {
        size_t lineEnd = text.find('\n', cursor);
        if (lineEnd == std::string::npos) {
            lineEnd = text.size();
        }
        const std::string line = trim(text.substr(cursor, lineEnd - cursor));
        cursor = lineEnd + 1;

        if (line.empty()) {
            continue;
        }
        if (startsWith(line, "Filetype:")) {
            sawFiletype = true;
            continue;
        }
        // Container and data-format versions are keys this firmware writes
        // itself (see toFlipperNfc). They have to be consumed here rather than
        // preserved as "unknown": unknownKeys is re-emitted verbatim on save, so
        // treating them as unknown duplicates them on every save-load-save pass.
        if (startsWith(line, "Version:") || startsWith(line, "Data format version:")) {
            continue;
        }
        if (startsWith(line, "Device type:")) {
            dump.deviceType = trim(line.substr(12));
            if (startsWith(dump.deviceType, "Mifare Classic")) {
                dump.kind = CardDumpKind::MifareClassic;
            } else if (startsWith(dump.deviceType, "Mifare Ultralight")) {
                dump.kind = CardDumpKind::Ultralight;
            } else if (startsWith(dump.deviceType, "NTAG")) {
                dump.kind = CardDumpKind::Ntag21x;
            }
            continue;
        }
        if (startsWith(line, "UID:")) {
            std::vector<uint8_t> uid;
            if (!parseBytes(line.substr(4), uid) || uid.size() > dump.uid.size()) {
                result.detail = "bad uid";
                return result;
            }
            for (size_t index = 0; index < uid.size(); ++index) {
                dump.uid[index] = uid[index];
            }
            dump.uidLength = static_cast<uint8_t>(uid.size());
            continue;
        }
        if (startsWith(line, "ATQA:")) {
            std::vector<uint8_t> bytes;
            if (!parseBytes(line.substr(5), bytes) || bytes.size() != 2) {
                result.detail = "bad atqa";
                return result;
            }
            dump.atqa = static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
            continue;
        }
        if (startsWith(line, "SAK:")) {
            std::vector<uint8_t> bytes;
            if (!parseBytes(line.substr(4), bytes) || bytes.empty()) {
                result.detail = "bad sak";
                return result;
            }
            dump.sak = bytes[0];
            continue;
        }
        if (startsWith(line, "Mifare Classic type:")) {
            dump.mifareType = trim(line.substr(20));
            continue;
        }

        size_t index = 0;
        BlockPayload payload;
        if (parseBlockLine(line, index, payload)) {
            if (dump.blocks.size() <= index) {
                dump.blocks.resize(index + 1);
            }
            CardDumpBlock& block = dump.blocks[index];
            if (payload.unknown) {
                // Keep the slot even though it is unread: dropping it would
                // renumber every later block and silently corrupt the dump.
                block.known = false;
            } else if (payload.bytes.size() == kCardDumpBlockSize) {
                for (size_t byte = 0; byte < kCardDumpBlockSize; ++byte) {
                    block.data[byte] = payload.bytes[byte];
                }
                block.known = true;
            } else {
                result.detail = "bad block payload";
                return result;
            }
            continue;
        }

        // Anything else is a key this build does not model; keep it verbatim so
        // re-saving does not drop it.
        dump.unknownKeys.push_back(line);
    }

    if (!sawFiletype) {
        result.detail = "missing filetype";
        return result;
    }
    if (dump.uidLength == 0 || dump.blocks.empty()) {
        result.detail = "incomplete dump";
        return result;
    }
    result.valid = true;
    result.detail = "flipper nfc loaded";
    return result;
}

CardDumpResult CardDumpFormat::fromProxmarkEml(const std::string& text) {
    CardDumpResult result;
    CardDump& dump = result.dump;
    dump.kind = CardDumpKind::MifareClassic;
    dump.deviceType = "Mifare Classic";

    size_t cursor = 0;
    while (cursor < text.size()) {
        size_t lineEnd = text.find('\n', cursor);
        if (lineEnd == std::string::npos) {
            lineEnd = text.size();
        }
        const std::string line = trim(text.substr(cursor, lineEnd - cursor));
        cursor = lineEnd + 1;

        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::vector<uint8_t> bytes;
        if (!parseBytes(line, bytes)) {
            result.detail = "bad block payload";
            return result;
        }
        if (bytes.empty()) {
            continue;
        }
        if (bytes.size() != kCardDumpBlockSize) {
            result.detail = "block is not 16 bytes";
            return result;
        }
        CardDumpBlock block;
        for (size_t byte = 0; byte < kCardDumpBlockSize; ++byte) {
            block.data[byte] = bytes[byte];
        }
        dump.blocks.push_back(block);
    }

    if (dump.blocks.empty()) {
        result.detail = "no blocks";
        return result;
    }
    // A Proxmark dump has no header, so take UID/ATQA/SAK from block 0, which is
    // the manufacturer block on every MIFARE Classic card.
    const CardDumpBlock& manufacturer = dump.blocks[0];
    const uint8_t uidLength = 4;
    for (uint8_t index = 0; index < uidLength; ++index) {
        dump.uid[index] = manufacturer.data[index];
    }
    dump.uidLength = uidLength;
    dump.atqa = static_cast<uint16_t>(manufacturer.data[6] | (manufacturer.data[7] << 8));
    dump.sak = manufacturer.data[5];
    dump.mifareType = dump.blocks.size() >= 256 ? "4K" : (dump.blocks.size() >= 64 ? "1K" : "Mini");
    result.valid = true;
    result.detail = "proxmark eml loaded";
    return result;
}

CardDumpResult CardDumpFormat::parseByExtension(const std::string& fileName, const std::string& text) {
    const size_t dot = fileName.find_last_of('.');
    const std::string extension = dot == std::string::npos ? std::string() : fileName.substr(dot + 1);
    if (extension == "eml") {
        return fromProxmarkEml(text);
    }
    if (extension == "nfc") {
        return fromFlipperNfc(text);
    }
    CardDumpResult result;
    result.detail = "unsupported dump extension";
    return result;
}
