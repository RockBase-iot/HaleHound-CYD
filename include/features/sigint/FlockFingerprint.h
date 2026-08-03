#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class IFeature;

struct FlockObservation {
    std::string oui;
    std::vector<std::string> serviceUuids;
    bool gpsValid = false;
    double latitude = 0.0;
    double longitude = 0.0;
};

struct FlockMatch {
    bool matched = false;
    uint8_t confidence = 0;
    bool gpsTagged = false;
    const char* lootPath = "/loot/flock";
};

class FlockFingerprint {
public:
    static size_t knownOuiCount();
    static bool isKnownOui(const std::string& oui);
    static const char* ravenServiceUuid();
    static FlockMatch evaluate(const FlockObservation& observation);
};

IFeature* createFlockYouFeature();
