#include "services/SettingsService.h"

namespace settings_record {
namespace {
constexpr size_t kCrcOffset = offsetof(SettingsRecord, crc);
}

uint32_t crc32(const SettingsRecord& record) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(&record);
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t index = 0; index < kCrcOffset; ++index) {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xEDB88320U : (crc >> 1U);
        }
    }
    return ~crc;
}

SettingsRecord make(uint32_t sequence, const SettingsData& data) {
    SettingsRecord record{};
    record.magic = kMagic;
    record.schema = kSchema;
    record.length = static_cast<uint8_t>(sizeof(SettingsRecord));
    record.sequence = sequence;
    record.disclaimerAccepted = data.disclaimerAccepted ? 1U : 0U;
    record.blueTeamMode = data.blueTeamMode ? 1U : 0U;
    record.crc = crc32(record);
    return record;
}

bool valid(const SettingsRecord& record) {
    return record.magic == kMagic && record.schema == kSchema &&
           record.length == sizeof(SettingsRecord) && record.crc == crc32(record) &&
           record.disclaimerAccepted <= 1U && record.blueTeamMode <= 1U;
}

SettingsData decode(const SettingsRecord& record) {
    if (!valid(record)) {
        return SettingsData{};
    }
    // No normalisation here anymore. The three policy states are carried
    // verbatim by the two bits; collapsing (false,false) into Blue Team used to
    // make the "never decided" state unrepresentable, which forced the
    // disclaimer to be unreachable on a fresh device.
    SettingsData data;
    data.disclaimerAccepted = record.disclaimerAccepted == 1U;
    data.blueTeamMode = record.blueTeamMode == 1U;
    return data;
}

bool newer(const SettingsRecord& left, const SettingsRecord& right) {
    return static_cast<int32_t>(left.sequence - right.sequence) > 0;
}

SettingsData migrateLegacy(bool disclaimerAccepted, bool blueTeamMode) {
    // Legacy firmware stored two raw booleans and never wrote (false,false).
    // Map them faithfully so a device that had never accepted stays Undecided
    // instead of being silently promoted into a sticky Blue Team.
    return SettingsData{disclaimerAccepted, blueTeamMode};
}
}
