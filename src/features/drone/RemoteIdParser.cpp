#include "features/drone/RemoteIdParser.h"

#include <cstring>

namespace {
int32_t readLittleEndian(const uint8_t* data) {
    return static_cast<int32_t>(static_cast<uint32_t>(data[0]) |
                                (static_cast<uint32_t>(data[1]) << 8U) |
                                (static_cast<uint32_t>(data[2]) << 16U) |
                                (static_cast<uint32_t>(data[3]) << 24U));
}
}

RemoteIdParseResult RemoteIdParser::parse(const std::vector<uint8_t>& frame,
                                          RemoteIdTransport transport) {
    RemoteIdParseResult result;
    if (frame.size() < 2U || frame[0] > 1U || frame[1] == 0U || frame[1] > 20U ||
        frame.size() < static_cast<size_t>(2U + frame[1])) {
        result.detail = "unsupported or truncated Remote ID frame";
        return result;
    }
    result.record.messageType = frame[0];
    result.record.serial.assign(reinterpret_cast<const char*>(&frame[2]), frame[1]);
    result.record.transportMask = transport == RemoteIdTransport::Wifi ? 1U : 2U;
    const size_t payloadStart = 2U + frame[1];
    if (frame.size() >= payloadStart + 9U && frame[payloadStart] == 1U) {
        result.record.hasLocation = true;
        result.record.latitudeE6 = readLittleEndian(&frame[payloadStart + 1U]);
        result.record.longitudeE6 = readLittleEndian(&frame[payloadStart + 5U]);
    }
    result.valid = true;
    result.detail = "Remote ID fixture parsed";
    return result;
}

void RemoteIdAggregator::observe(const RemoteIdRecord& record) {
    if (record.serial.empty()) return;
    for (auto& existing : records_) {
        if (existing.serial == record.serial) {
            existing.transportMask = static_cast<uint8_t>(existing.transportMask | record.transportMask);
            if (record.hasLocation) {
                existing.hasLocation = true;
                existing.latitudeE6 = record.latitudeE6;
                existing.longitudeE6 = record.longitudeE6;
            }
            return;
        }
    }
    records_.push_back(record);
}

const RemoteIdRecord& RemoteIdAggregator::find(const std::string& serial) const {
    static const RemoteIdRecord empty{};
    for (const auto& record : records_) {
        if (record.serial == serial) return record;
    }
    return empty;
}
