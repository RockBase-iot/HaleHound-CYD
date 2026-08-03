#pragma once

#include <cstdint>

#include "core/FeatureTypes.h"
#include "features/FeatureCatalog.h"
#include "services/SettingsService.h"

enum class PolicyMode : uint8_t {
    BlueTeam = 0,
    ValhallaAccepted = 1,
};

enum class AuthorizationCode : uint8_t {
    Allowed,
    Blocked,
};

struct Authorization {
    AuthorizationCode code;
    const char* detail;

    bool allowed() const { return code == AuthorizationCode::Allowed; }
};

class PolicyService {
public:
    explicit PolicyService(SettingsService& settings);

    Authorization authorize(const FeatureDescriptor& feature) const;
    bool acceptValhalla();
    bool enterBlueTeam();
    PolicyMode mode() const { return mode_; }
    const char* modeLabel() const;

    static bool legacyAllowsOffensive(bool disclaimerAccepted,
                                      bool blueTeamMode);

private:
    void refresh();

    SettingsService& settings_;
    PolicyMode mode_ = PolicyMode::BlueTeam;
};
