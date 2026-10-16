#pragma once

#include <cstdint>

#include "core/FeatureTypes.h"
#include "features/FeatureCatalog.h"
#include "services/SettingsService.h"

// VALHALLA policy state. Three states, not two booleans:
//
//   Undecided        : nothing chosen yet. Offensive tools raise the disclaimer.
//   ValhallaAccepted : the user accepted the liability disclaimer. Offensive tools run.
//   BlueTeam         : the user declined (or deliberately engaged Blue Team).
//                      Offensive tools are locked and there is exactly ONE way out:
//                      an explicit, separate leaveBlueTeam(true) call.
//
// The previous design stored two independent booleans and let the disclaimer
// screen clear `blueTeamMode` on ACCEPT. That made Blue Team self-defeating:
// block screen -> tap tool -> ACCEPT -> Blue Team gone. BlueTeam is now sticky;
// no disclaimer interaction can leave it.
enum class PolicyMode : uint8_t {
    Undecided = 0,
    ValhallaAccepted = 1,
    BlueTeam = 2,
};

enum class AuthorizationCode : uint8_t {
    Allowed,
    Blocked,
};

// What the caller is expected to do with a Blocked decision. Returning the
// reason (not a bool) is what lets every gate collapse into one call.
enum class PolicyAction : uint8_t {
    Proceed,            // run the feature
    PromptValhalla,     // show the disclaimer; only ACCEPT unlocks
    BlockedBlueTeam,    // show the Blue Team block screen; nothing unlocks
};

struct Authorization {
    AuthorizationCode code;
    PolicyAction action;
    const char* detail;

    bool allowed() const { return code == AuthorizationCode::Allowed; }
};

class PolicyService {
public:
    explicit PolicyService(SettingsService& settings);

    // ─── Authorization ─────────────────────────────────────────────────────
    Authorization authorize(const FeatureDescriptor& feature) const;
    // Convenience for the legacy menu: offensive-by-definition call sites.
    Authorization authorizeOffensive() const;
    bool offensiveAllowed() const;

    // ─── State transitions (the only writers of policy state) ──────────────
    // Accepts the liability disclaimer. REFUSED while in Blue Team: returns
    // false and leaves the persisted state untouched. This is the single
    // change that closes the "Blue Team bypassed by re-accepting" hole.
    bool acceptValhalla();
    // Decline: persistent Blue Team. Reachable from Undecided only.
    bool declineValhalla();
    // Engage Blue Team deliberately (panic button / explicit request).
    // Preserves the disclaimer bit so the stored record still reflects that
    // the user had accepted at some point.
    bool enterBlueTeam();
    // Emergency lockdown for destructive flows: engage Blue Team AND clear the
    // disclaimer bit, i.e. "as if never accepted". Distinct from enterBlueTeam
    // so the two intents cannot be confused.
    bool lockDown();
    // The ONLY exit from Blue Team. Requires an explicit risk acknowledgement;
    // callers that pass false get a refusal with no state change.
    bool leaveBlueTeam(bool acknowledgedRisk);

    // ─── Introspection ─────────────────────────────────────────────────────
    PolicyMode mode() const { return mode_; }
    const char* modeLabel() const;
    bool inBlueTeam() const { return mode_ == PolicyMode::BlueTeam; }
    bool disclaimerAccepted() const { return disclaimerAccepted_; }

    // Re-read the store. Needed when something outside this service wrote the
    // record (legacy migration on boot).
    void syncFromStore();

    // ─── Pure helpers (unit-testable without a store) ──────────────────────
    static PolicyMode modeFor(bool disclaimerAccepted, bool blueTeamMode);
    static PolicyAction actionFor(PolicyMode mode, RiskClass risk);
    static bool isOffensive(RiskClass risk);

    // Legacy mapping retained for存档迁移 and for old call sites that only
    // have the two raw booleans. Prefer authorize()/offensiveAllowed().
    static bool legacyAllowsOffensive(bool disclaimerAccepted,
                                      bool blueTeamMode);

private:
    void refresh();

    SettingsService& settings_;
    PolicyMode mode_ = PolicyMode::BlueTeam;
    bool disclaimerAccepted_ = false;
};

// The process-wide settings store and policy instance, in the shape
// `globalBootHealthService()` established: a function-local static behind an
// accessor, so nothing can end up holding a second policy with its own store.
//
// They live in the core rather than in the legacy app because both the legacy
// menu -- which renders the disclaimer and the Blue Team screen -- and the
// FeatureRunner's authorization hook -- which decides whether a feature may
// start at all -- must be asking the *same* authority. Two instances reading
// two stores is precisely the shape of the bug this service exists to prevent,
// and it is the shape the wiring had while the store lived inside the menu.
//
// On a host build the store is a backend that refuses every read and write, so
// the service comes up Undecided and transient -- which is the honest state of
// a machine with no EEPROM, not a simulation of one.
SettingsService& globalSettingsStore();
PolicyService& globalPolicyService();
