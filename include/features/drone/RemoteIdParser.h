#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IFeature;

enum class RemoteIdTransport : uint8_t {
    Wifi,
    Ble,
};

struct RemoteIdRecord {
    uint8_t messageType = 0;
    std::string serial;
    bool hasLocation = false;
    int32_t latitudeE6 = 0;
    int32_t longitudeE6 = 0;
    uint8_t transportMask = 0;
};

struct RemoteIdParseResult {
    bool valid = false;
    const char* detail = "invalid Remote ID frame";
    RemoteIdRecord record;
};

class RemoteIdParser {
public:
    static RemoteIdParseResult parse(const std::vector<uint8_t>& frame,
                                     RemoteIdTransport transport);
};

class RemoteIdAggregator {
public:
    void observe(const RemoteIdRecord& record);
    size_t size() const { return records_.size(); }
    const RemoteIdRecord& find(const std::string& serial) const;

private:
    std::vector<RemoteIdRecord> records_;
};

IFeature* createDroneDetectFeature();
IFeature* createRidSpoofFeature();
IFeature* createDroneJellyFeature();
IFeature* createDroneDeauthFeature();
