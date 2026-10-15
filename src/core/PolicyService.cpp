#include "core/PolicyService.h"

namespace {
Authorization allow(const char* detail) {
    return {AuthorizationCode::Allowed, PolicyAction::Proceed, detail};
}
}  // namespace

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
