#include "core/PolicyService.h"

#ifdef ARDUINO
#include <EEPROM.h>
#include <cstddef>
#include <cstring>
#endif

namespace {
Authorization allow(const char* detail) {
    return {AuthorizationCode::Allowed, PolicyAction::Proceed, detail};
}
}  // namespace

#ifdef ARDUINO
namespace {
// Reserved EEPROM region: two slots of sizeof(SettingsRecord), at a fixed offset
// so the legacy single-blob settings cannot collide with the policy record.
constexpr int kPolicyEepromOffset = 128;
constexpr int kPolicyEepromSize = 512;

int policySlotOffset(uint8_t slot) {
    return kPolicyEepromOffset +
           static_cast<int>(slot) * static_cast<int>(sizeof(SettingsRecord));
}

// EEPROM-backed ISettingsBackend for the policy region only. Deliberately
// separate from the legacy Settings struct so PolicyService can own its own
// record (magic + schema + CRC + sequence) without colliding with the old
// single-blob layout.
class EepromPolicyBackend final : public ISettingsBackend {
public:
    bool read(uint8_t slot, void* data, size_t size) override {
        if (slot > 1U || data == nullptr || size != sizeof(SettingsRecord)) {
            return false;
        }
        SettingsRecord record{};
        EEPROM.begin(kPolicyEepromSize);
        EEPROM.get(policySlotOffset(slot), record);
        EEPROM.end();
        std::memcpy(data, &record, sizeof(record));
        return true;
    }

    bool write(uint8_t slot, const void* data, size_t size) override {
        if (slot > 1U || data == nullptr || size != sizeof(SettingsRecord)) {
            return false;
        }
        SettingsRecord record{};
        std::memcpy(&record, data, sizeof(record));
        EEPROM.begin(kPolicyEepromSize);
        EEPROM.put(policySlotOffset(slot), record);
        const bool ok = EEPROM.commit();
        EEPROM.end();
        return ok;
    }

    bool commit() override {
        EEPROM.begin(kPolicyEepromSize);
        const bool ok = EEPROM.commit();
        EEPROM.end();
        return ok;
    }
};

EepromPolicyBackend policyBackend;
}  // namespace
#else
namespace {
// No EEPROM on a host build. Every operation refuses, so nothing is ever
// persisted and the service comes up Undecided -- a machine with no store, not
// a pretend store that silently accepts writes and forgets them.
class UnavailableSettingsBackend final : public ISettingsBackend {
public:
    bool read(uint8_t, void*, size_t) override { return false; }
    bool write(uint8_t, const void*, size_t) override { return false; }
    bool commit() override { return false; }
};

UnavailableSettingsBackend policyBackend;
}  // namespace
#endif

SettingsService& globalSettingsStore() {
    static SettingsService store(policyBackend);
    return store;
}

PolicyService& globalPolicyService() {
    static PolicyService service(globalSettingsStore());
    return service;
}

PolicyService::PolicyService(SettingsService& settings) : settings_(settings) {
    if (!settings_.loaded()) {
        settings_.load();
    }
    refresh();
}

bool PolicyService::isOffensive(RiskClass risk) {
    return risk == RiskClass::Offensive || risk == RiskClass::SensitiveData;
}

PolicyMode PolicyService::modeFor(bool disclaimerAccepted, bool blueTeamMode) {
    // Blue Team wins over everything: it is sticky by definition. The
    // disclaimer bit is retained only so a stored record still says whether
    // the user had accepted before entering Blue Team.
    if (blueTeamMode) {
        return PolicyMode::BlueTeam;
    }
    return disclaimerAccepted ? PolicyMode::ValhallaAccepted : PolicyMode::Undecided;
}

PolicyAction PolicyService::actionFor(PolicyMode mode, RiskClass risk) {
    if (!isOffensive(risk)) {
        return PolicyAction::Proceed;
    }
    switch (mode) {
        case PolicyMode::ValhallaAccepted:
            return PolicyAction::Proceed;
        case PolicyMode::Undecided:
            return PolicyAction::PromptValhalla;
        case PolicyMode::BlueTeam:
        default:
            return PolicyAction::BlockedBlueTeam;
    }
}

void PolicyService::refresh() {
    const SettingsData& data = settings_.data();
    disclaimerAccepted_ = data.disclaimerAccepted;
    mode_ = modeFor(data.disclaimerAccepted, data.blueTeamMode);
}

void PolicyService::syncFromStore() {
    if (settings_.load()) {
        refresh();
    }
}

Authorization PolicyService::authorize(const FeatureDescriptor& feature) const {
    const PolicyAction action = actionFor(mode_, feature.risk);
    switch (action) {
        case PolicyAction::Proceed:
            return allow(isOffensive(feature.risk) ? "VALHALLA accepted"
                                                   : "passive or defensive feature");
        case PolicyAction::PromptValhalla:
            return {AuthorizationCode::Blocked,
                    PolicyAction::PromptValhalla,
                    "VALHALLA responsibility disclaimer required"};
        case PolicyAction::BlockedBlueTeam:
        default:
            return {AuthorizationCode::Blocked,
                    PolicyAction::BlockedBlueTeam,
                    "Blue Team mode active - leave via Device Policy"};
    }
}

Authorization PolicyService::authorizeOffensive() const {
    return authorize(FeatureDescriptor{FeatureId::WifiDeauth, CategoryId::Wifi,
                                       "offensive", RiskClass::Offensive,
                                       {}, 0, {}, 0, "", "", nullptr});
}

bool PolicyService::offensiveAllowed() const {
    return actionFor(mode_, RiskClass::Offensive) == PolicyAction::Proceed;
}

bool PolicyService::acceptValhalla() {
    if (mode_ == PolicyMode::BlueTeam) {
        // Blue Team is sticky. Re-accepting the disclaimer must NOT unlock it;
        // that was the bypass this service now owns and refuses.
        return false;
    }
    const bool saved = settings_.save(SettingsData{true, false});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::ValhallaAccepted;
}

bool PolicyService::declineValhalla() {
    const bool saved = settings_.save(SettingsData{disclaimerAccepted_, true});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::BlueTeam;
}

bool PolicyService::enterBlueTeam() {
    const bool saved = settings_.save(SettingsData{disclaimerAccepted_, true});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::BlueTeam;
}

bool PolicyService::leaveBlueTeam(bool acknowledgedRisk) {
    if (!acknowledgedRisk) {
        // No acknowledgement, no exit. This is the deliberate counterpart to
        // the sticky Blue Team state.
        return false;
    }
    const bool saved = settings_.save(SettingsData{true, false});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::ValhallaAccepted;
}

bool PolicyService::lockDown() {
    const bool saved = settings_.save(SettingsData{false, true});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::BlueTeam;
}

const char* PolicyService::modeLabel() const {
    switch (mode_) {
        case PolicyMode::ValhallaAccepted:
            return "VALHALLA";
        case PolicyMode::Undecided:
            return "VALHALLA";
        case PolicyMode::BlueTeam:
        default:
            return "BLUE TEAM";
    }
}

bool PolicyService::legacyAllowsOffensive(bool disclaimerAccepted,
                                          bool blueTeamMode) {
    return modeFor(disclaimerAccepted, blueTeamMode) ==
           PolicyMode::ValhallaAccepted;
}
