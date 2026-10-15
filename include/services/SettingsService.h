#pragma once

#include <cstddef>
#include <cstdint>

// Two persisted bits, three policy states:
//   (false, false) = Undecided        - offensive locked, disclaimer will be offered
//   (true,  false) = ValhallaAccepted - offensive allowed
//   (*,     true)  = BlueTeam         - offensive locked, sticky
//
// The default is Undecided, NOT Blue Team: a device that has never been
// configured must still be able to accept the disclaimer. Blue Team is only
// ever entered by an explicit decline/engage, and only left by an explicit
// acknowledged request (see PolicyService).
struct SettingsData {
    constexpr SettingsData(bool disclaimer = false, bool blueTeam = false)
        : disclaimerAccepted(disclaimer), blueTeamMode(blueTeam) {}

    bool disclaimerAccepted;
    bool blueTeamMode;
};

#pragma pack(push, 1)
struct SettingsRecord {
    uint16_t magic;
    uint8_t schema;
    uint8_t length;
    uint32_t sequence;
    uint8_t disclaimerAccepted;
    uint8_t blueTeamMode;
    uint16_t reserved;
    uint32_t crc;
};
#pragma pack(pop)

class ISettingsBackend {
public:
    virtual ~ISettingsBackend() = default;
    virtual bool read(uint8_t slot, void* data, size_t size) = 0;
    virtual bool write(uint8_t slot, const void* data, size_t size) = 0;
    virtual bool commit() = 0;
};

namespace settings_record {
constexpr uint16_t kMagic = 0x4848U;
constexpr uint8_t kSchema = 1U;

uint32_t crc32(const SettingsRecord& record);
SettingsRecord make(uint32_t sequence, const SettingsData& data);
bool valid(const SettingsRecord& record);
SettingsData decode(const SettingsRecord& record);
bool newer(const SettingsRecord& left, const SettingsRecord& right);
SettingsData migrateLegacy(bool disclaimerAccepted, bool blueTeamMode);
}

class SettingsService {
public:
    explicit SettingsService(ISettingsBackend& backend) : backend_(backend) {}

    bool load();
    bool save(const SettingsData& data);
    bool migrateLegacy(bool disclaimerAccepted, bool blueTeamMode);
    const SettingsData& data() const { return data_; }
    bool loaded() const { return loaded_; }
    // True when at least one slot held a valid, CRC-checked record. Used by the
    // one-time migration path: an empty store means "adopt the legacy bytes".
    bool hasStoredRecord() const { return hasRecord_; }

private:
    ISettingsBackend& backend_;
    SettingsData data_{};
    SettingsRecord current_{};
    uint8_t currentSlot_ = 0;
    bool loaded_ = false;
    bool hasRecord_ = false;
};
