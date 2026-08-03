#include "core/PolicyService.h"

PolicyService::PolicyService(SettingsService& settings) : settings_(settings) {
    if (!settings_.loaded()) {
        settings_.load();
    }
    refresh();
}

void PolicyService::refresh() {
    const SettingsData& data = settings_.data();
    mode_ = data.disclaimerAccepted && !data.blueTeamMode
                 ? PolicyMode::ValhallaAccepted
                 : PolicyMode::BlueTeam;
}

Authorization PolicyService::authorize(const FeatureDescriptor& feature) const {
    if (feature.risk == RiskClass::Passive || feature.risk == RiskClass::Defensive) {
        return {AuthorizationCode::Allowed, "passive or defensive feature"};
    }
    if (mode_ != PolicyMode::ValhallaAccepted) {
        return {AuthorizationCode::Blocked,
                "VALHALLA responsibility disclaimer required"};
    }
    return {AuthorizationCode::Allowed, "VALHALLA accepted"};
}

bool PolicyService::acceptValhalla() {
    const bool saved = settings_.save(SettingsData{true, false});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::ValhallaAccepted;
}

bool PolicyService::enterBlueTeam() {
    const bool saved = settings_.save(SettingsData{false, true});
    if (saved) {
        refresh();
    }
    return saved && mode_ == PolicyMode::BlueTeam;
}

const char* PolicyService::modeLabel() const {
    return mode_ == PolicyMode::ValhallaAccepted ? "VALHALLA" : "BLUE TEAM";
}

bool PolicyService::legacyAllowsOffensive(bool disclaimerAccepted,
                                          bool blueTeamMode) {
    return disclaimerAccepted && !blueTeamMode;
}
