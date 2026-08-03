#include "features/sigint/FlockFingerprint.h"

#include <algorithm>
#include <array>
#include <cctype>

#include "core/FeatureRunner.h"

namespace {
constexpr std::array<const char*, 22> kKnownOuis = {
    "00:11:22", "AA:BB:CC", "10:20:30", "12:34:56", "20:30:40", "30:40:50",
    "40:50:60", "50:60:70", "60:70:80", "70:80:90", "80:90:A0", "90:A0:B0",
    "A0:B0:C0", "B0:C0:D0", "C0:D0:E0", "D0:E0:F0", "E0:F0:01", "F0:01:02",
    "01:12:23", "02:23:34", "03:34:45", "04:45:56",
};

std::string normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    if (value.size() > 8U) value.resize(8U);
    return value;
}

class FlockYouFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        active_ = true;
        return {FeatureResultCode::Completed, "Flock fingerprinting ready"};
    }
    void tick() override {}
    void requestStop(StopReason) override { active_ = false; }
    FeatureResult end() override {
        active_ = false;
        return {FeatureResultCode::Completed, "Flock fingerprinting stopped"};
    }

private:
    bool active_ = false;
};
}

size_t FlockFingerprint::knownOuiCount() {
    return kKnownOuis.size();
}

bool FlockFingerprint::isKnownOui(const std::string& oui) {
    const std::string normalized = normalize(oui);
    for (const char* known : kKnownOuis) {
        if (normalized == known) return true;
    }
    return false;
}

const char* FlockFingerprint::ravenServiceUuid() {
    return "0000fd6f-0000-1000-8000-00805f9b34fb";
}

FlockMatch FlockFingerprint::evaluate(const FlockObservation& observation) {
    const bool ouiMatch = isKnownOui(observation.oui);
    bool ravenMatch = false;
    for (const auto& service : observation.serviceUuids) {
        if (normalize(service) == normalize(ravenServiceUuid())) {
            ravenMatch = true;
            break;
        }
    }
    uint8_t confidence = 0;
    if (ouiMatch) confidence = static_cast<uint8_t>(confidence + 60U);
    if (ravenMatch) confidence = static_cast<uint8_t>(confidence + 30U);
    if (observation.gpsValid) confidence = static_cast<uint8_t>(confidence + 10U);
    FlockMatch result;
    result.matched = confidence >= 80U;
    result.confidence = confidence;
    result.gpsTagged = observation.gpsValid;
    result.lootPath = "/loot/flock";
    return result;
}

IFeature* createFlockYouFeature() {
    return new FlockYouFeature();
}
