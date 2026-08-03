#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IFeature;

enum class SubProtocol : uint8_t {
    Raw,
    Princeton,
    Came,
    Nice,
    Unknown,
};

struct SubPulse {
    uint32_t durationUs = 0;
    bool level = false;
};

struct SubFileDocument {
    uint32_t frequencyHz = 0;
    SubProtocol protocol = SubProtocol::Unknown;
    uint64_t key = 0;
    uint16_t bitCount = 0;
    uint32_t teUs = 0;
    std::vector<SubPulse> pulses;
    std::string sourcePath;
    bool transmitAllowed = false;
};

struct SubParseResult {
    bool valid = false;
    const char* detail = "invalid sub file";
    SubFileDocument document;
};

struct SubFileInput {
    std::string path;
    std::string contents;
};

struct SubDirectoryResult {
    bool valid = false;
    const char* detail = "invalid sub directory";
    std::vector<SubFileDocument> documents;
};

class SubFileParser {
public:
    static SubParseResult parse(const std::string& contents,
                                const std::string& sourcePath = {});
    static SubDirectoryResult parseDirectory(const std::vector<SubFileInput>& files);
};

IFeature* createSubReadFeature();
IFeature* createTeslaChargeFeature();
