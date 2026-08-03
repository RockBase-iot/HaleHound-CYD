#include "features/subghz/SubFileParser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace {
constexpr uint32_t kMaxPulseUs = 2000000U;

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

bool parseUnsigned(const std::string& text, uint64_t& value) {
    std::string token = trim(text);
    int base = 10;
    if (token.size() > 2U && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
        base = 16;
        token.erase(0U, 2U);
    }
    if (token.empty()) {
        return false;
    }
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(token.c_str(), &end, base);
    if (end == token.c_str() || *end != '\0') {
        return false;
    }
    value = static_cast<uint64_t>(parsed);
    return true;
}

SubProtocol parseProtocol(const std::string& value) {
    const std::string protocol = lower(trim(value));
    if (protocol == "raw") return SubProtocol::Raw;
    if (protocol == "princeton") return SubProtocol::Princeton;
    if (protocol == "came") return SubProtocol::Came;
    if (protocol == "nice") return SubProtocol::Nice;
    return SubProtocol::Unknown;
}

bool hasParentTraversal(const std::string& path) {
    size_t start = 0U;
    while (start < path.size()) {
        const size_t end = path.find_first_of("/\\", start);
        const std::string segment = path.substr(start, end == std::string::npos ? end : end - start);
        if (segment == "..") return true;
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return false;
}
}

SubParseResult SubFileParser::parse(const std::string& contents,
                                    const std::string& sourcePath) {
    SubParseResult result;
    result.document.sourcePath = sourcePath;
    if (contents.empty()) {
        result.detail = "empty sub file";
        return result;
    }

    bool hasFrequency = false;
    bool hasProtocol = false;
    bool hasKey = false;
    bool hasBits = false;
    bool hasTe = false;
    bool level = true;
    std::istringstream stream(contents);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const size_t separator = line.find(':');
        if (separator == std::string::npos) continue;
        const std::string field = lower(trim(line.substr(0U, separator)));
        const std::string value = trim(line.substr(separator + 1U));
        uint64_t parsed = 0U;
        if (field == "frequency") {
            if (!parseUnsigned(value, parsed) || parsed == 0U || parsed > 1000000000ULL) {
                result.detail = "invalid frequency";
                return result;
            }
            result.document.frequencyHz = static_cast<uint32_t>(parsed);
            hasFrequency = true;
        } else if (field == "protocol") {
            result.document.protocol = parseProtocol(value);
            hasProtocol = true;
        } else if (field == "key") {
            if (!parseUnsigned(value, parsed)) {
                result.detail = "invalid key";
                return result;
            }
            result.document.key = parsed;
            hasKey = true;
        } else if (field == "bit" || field == "bits") {
            if (!parseUnsigned(value, parsed) || parsed == 0U || parsed > 128U) {
                result.detail = "invalid bit count";
                return result;
            }
            result.document.bitCount = static_cast<uint16_t>(parsed);
            hasBits = true;
        } else if (field == "te" || field == "te_us") {
            if (!parseUnsigned(value, parsed) || parsed == 0U || parsed > kMaxPulseUs) {
                result.detail = "invalid timing";
                return result;
            }
            result.document.teUs = static_cast<uint32_t>(parsed);
            hasTe = true;
        } else if (field == "raw_data") {
            std::istringstream pulses(value);
            std::string token;
            while (pulses >> token) {
                char* end = nullptr;
                const long long signedPulse = std::strtoll(token.c_str(), &end, 10);
                if (end == token.c_str() || *end != '\0' || signedPulse == 0LL) {
                    result.detail = "invalid raw pulse";
                    return result;
                }
                const uint64_t magnitude = signedPulse < 0LL
                                               ? static_cast<uint64_t>(-(signedPulse + 1LL)) + 1ULL
                                               : static_cast<uint64_t>(signedPulse);
                if (magnitude > kMaxPulseUs) {
                    result.detail = "raw pulse exceeds safety bound";
                    return result;
                }
                SubPulse pulse;
                pulse.durationUs = static_cast<uint32_t>(magnitude);
                pulse.level = level;
                result.document.pulses.push_back(pulse);
                level = !level;
            }
        }
    }

    if (!hasFrequency || !hasProtocol) {
        result.detail = "required field missing";
        return result;
    }
    if (result.document.protocol == SubProtocol::Unknown) {
        result.detail = "unsupported protocol";
        return result;
    }
    if (result.document.protocol == SubProtocol::Raw) {
        if (result.document.pulses.empty()) {
            result.detail = "raw data missing";
            return result;
        }
    } else if (!hasKey || !hasBits || !hasTe) {
        result.detail = "structured protocol field missing";
        return result;
    }
    result.valid = true;
    result.detail = "parsed without transmit authorization";
    return result;
}

SubDirectoryResult SubFileParser::parseDirectory(const std::vector<SubFileInput>& files) {
    SubDirectoryResult result;
    result.valid = true;
    result.detail = "directory parsed";
    for (const auto& file : files) {
        if (hasParentTraversal(file.path)) {
            result.valid = false;
            result.detail = "parent traversal rejected";
            result.documents.clear();
            return result;
        }
        const std::string lowerPath = lower(file.path);
        if (lowerPath.size() < 4U || lowerPath.substr(lowerPath.size() - 4U) != ".sub") {
            continue;
        }
        const auto parsed = parse(file.contents, file.path);
        if (parsed.valid) {
            result.documents.push_back(parsed.document);
        } else if (parsed.detail != std::string("unsupported protocol")) {
            result.valid = false;
            result.detail = parsed.detail;
            result.documents.clear();
            return result;
        }
    }
    return result;
}
