#include "services/SettingsService.h"

bool SettingsService::load() {
    SettingsRecord records[2]{};
    const bool validRecords[2] = {
        backend_.read(0U, &records[0], sizeof(records[0])) && settings_record::valid(records[0]),
        backend_.read(1U, &records[1], sizeof(records[1])) && settings_record::valid(records[1]),
    };
    if (!validRecords[0] && !validRecords[1]) {
        data_ = SettingsData{};
        current_ = settings_record::make(0U, data_);
        currentSlot_ = 0U;
        loaded_ = true;
        hasRecord_ = false;
        return true;
    }
    currentSlot_ = validRecords[1] && (!validRecords[0] ||
                                      settings_record::newer(records[1], records[0]))
                       ? 1U
                       : 0U;
    current_ = records[currentSlot_];
    data_ = settings_record::decode(current_);
    loaded_ = true;
    hasRecord_ = true;
    return true;
}

bool SettingsService::save(const SettingsData& data) {
    if (!loaded_ && !load()) {
        return false;
    }
    const uint8_t nextSlot = currentSlot_ == 0U ? 1U : 0U;
    const SettingsRecord next = settings_record::make(current_.sequence + 1U, data);
    if (!backend_.write(nextSlot, &next, sizeof(next)) || !backend_.commit()) {
        return false;
    }
    SettingsRecord verify{};
    if (!backend_.read(nextSlot, &verify, sizeof(verify)) || !settings_record::valid(verify)) {
        return false;
    }
    current_ = verify;
    currentSlot_ = nextSlot;
    data_ = settings_record::decode(verify);
    loaded_ = true;
    hasRecord_ = true;
    return true;
}

bool SettingsService::migrateLegacy(bool disclaimerAccepted, bool blueTeamMode) {
    return save(settings_record::migrateLegacy(disclaimerAccepted, blueTeamMode));
}
