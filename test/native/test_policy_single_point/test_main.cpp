#include <unity.h>

#include <cstring>

#include "core/PolicyService.h"
#include "services/SettingsService.h"

namespace {

class MemoryBackend final : public ISettingsBackend {
public:
    bool read(uint8_t slot, void* data, size_t size) override {
        if (!valid[slot] || size != sizeof(records[slot])) {
            return false;
        }
        std::memcpy(data, &records[slot], size);
        return true;
    }

    bool write(uint8_t slot, const void* data, size_t size) override {
        if (size != sizeof(records[slot])) {
            return false;
        }
        std::memcpy(&records[slot], data, size);
        valid[slot] = true;
        return true;
    }

    bool commit() override { return commitResult; }

    SettingsRecord records[2]{};
    bool valid[2]{false, false};
    bool commitResult = true;
};

FeatureDescriptor offensiveFeature() {
    return FeatureDescriptor{FeatureId::WifiDeauth, CategoryId::Wifi, "deauth",
                             RiskClass::Offensive, {}, 0, {}, 0, "", "", nullptr};
}

FeatureDescriptor passiveFeature() {
    return FeatureDescriptor{FeatureId::WifiScanner, CategoryId::Wifi, "scan",
                             RiskClass::Passive, {}, 0, {}, 0, "", "", nullptr};
}

FeatureDescriptor defensiveFeature() {
    return FeatureDescriptor{FeatureId::WifiScanner, CategoryId::Wifi, "detect",
                             RiskClass::Defensive, {}, 0, {}, 0, "", "", nullptr};
}

}  // namespace

// ─── The regression this whole change exists for ───────────────────────────

// Blue Team used to be cleared by re-accepting the disclaimer from the block
// screen. acceptValhalla() must now refuse outright.
void test_accept_valhalla_is_refused_while_in_blue_team() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());

    TEST_ASSERT_TRUE(policy.enterBlueTeam());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());

    TEST_ASSERT_FALSE(policy.acceptValhalla());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());
    TEST_ASSERT_FALSE(policy.offensiveAllowed());
}

// The exact old sequence: decline -> tap a blocked tool -> ACCEPT disclaimer.
void test_decline_then_accept_stays_in_blue_team() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());

    TEST_ASSERT_TRUE(policy.declineValhalla());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());

    TEST_ASSERT_FALSE(policy.acceptValhalla());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());
    TEST_ASSERT_FALSE(policy.acceptValhalla());

    // And it survives a reboot: the store was never rewritten.
    SettingsService reloaded(backend);
    PolicyService reloadedPolicy(reloaded);
    TEST_ASSERT_TRUE(reloaded.load());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, reloadedPolicy.mode());
}

// Blue Team must never hand the caller a disclaimer prompt — that prompt is
// exactly how the old bypass got reached.
void test_blue_team_blocks_without_offering_the_disclaimer() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(policy.enterBlueTeam());

    const Authorization auth = policy.authorize(offensiveFeature());
    TEST_ASSERT_FALSE(auth.allowed());
    TEST_ASSERT_EQUAL(AuthorizationCode::Blocked, auth.code);
    TEST_ASSERT_EQUAL(PolicyAction::BlockedBlueTeam, auth.action);
}

// ─── The one legitimate exit ───────────────────────────────────────────────

void test_leave_blue_team_requires_an_acknowledgement() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(policy.enterBlueTeam());

    TEST_ASSERT_FALSE(policy.leaveBlueTeam(false));
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());

    TEST_ASSERT_TRUE(policy.leaveBlueTeam(true));
    TEST_ASSERT_EQUAL(PolicyMode::ValhallaAccepted, policy.mode());
    TEST_ASSERT_TRUE(policy.offensiveAllowed());
    TEST_ASSERT_TRUE(policy.disclaimerAccepted());
}

// ─── Undecided is the fresh-device state, and it must still be unlockable ──

void test_undecided_prompts_then_unlocks() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_EQUAL(PolicyMode::Undecided, policy.mode());

    const Authorization auth = policy.authorize(offensiveFeature());
    TEST_ASSERT_EQUAL(PolicyAction::PromptValhalla, auth.action);
    TEST_ASSERT_FALSE(auth.allowed());

    TEST_ASSERT_TRUE(policy.acceptValhalla());
    TEST_ASSERT_EQUAL(PolicyMode::ValhallaAccepted, policy.mode());
    TEST_ASSERT_TRUE(policy.authorize(offensiveFeature()).allowed());
    TEST_ASSERT_EQUAL(PolicyAction::Proceed,
                      policy.authorize(offensiveFeature()).action);
}

void test_passive_and_defensive_are_allowed_in_every_mode() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());

    // Undecided
    TEST_ASSERT_TRUE(policy.authorize(passiveFeature()).allowed());
    TEST_ASSERT_TRUE(policy.authorize(defensiveFeature()).allowed());

    // Blue Team
    TEST_ASSERT_TRUE(policy.enterBlueTeam());
    TEST_ASSERT_TRUE(policy.authorize(passiveFeature()).allowed());
    TEST_ASSERT_TRUE(policy.authorize(defensiveFeature()).allowed());

    // VALHALLA
    TEST_ASSERT_TRUE(policy.leaveBlueTeam(true));
    TEST_ASSERT_TRUE(policy.authorize(passiveFeature()).allowed());
    TEST_ASSERT_TRUE(policy.authorize(defensiveFeature()).allowed());
}

// ─── Transitions that must stay distinct ───────────────────────────────────

void test_enter_blue_team_preserves_the_disclaimer_bit() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(policy.acceptValhalla());

    TEST_ASSERT_TRUE(policy.enterBlueTeam());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());
    TEST_ASSERT_TRUE(policy.disclaimerAccepted());

    SettingsService reloaded(backend);
    PolicyService reloadedPolicy(reloaded);
    TEST_ASSERT_TRUE(reloaded.load());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, reloadedPolicy.mode());
    TEST_ASSERT_TRUE(reloadedPolicy.disclaimerAccepted());
}

void test_lockdown_clears_the_disclaimer_and_engages_blue_team() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(policy.acceptValhalla());

    TEST_ASSERT_TRUE(policy.lockDown());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());
    TEST_ASSERT_FALSE(policy.disclaimerAccepted());
    TEST_ASSERT_FALSE(policy.offensiveAllowed());
}

// ─── Pure mapping helpers ──────────────────────────────────────────────────

void test_modeFor_collapses_two_bits_into_three_states() {
    TEST_ASSERT_EQUAL(PolicyMode::Undecided,
                      PolicyService::modeFor(false, false));
    TEST_ASSERT_EQUAL(PolicyMode::ValhallaAccepted,
                      PolicyService::modeFor(true, false));
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, PolicyService::modeFor(false, true));
    // Blue Team wins even when the disclaimer bit is still set.
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, PolicyService::modeFor(true, true));
}

void test_actionFor_is_a_pure_function_of_mode_and_risk() {
    TEST_ASSERT_EQUAL(PolicyAction::Proceed,
                      PolicyService::actionFor(PolicyMode::BlueTeam,
                                               RiskClass::Passive));
    TEST_ASSERT_EQUAL(PolicyAction::BlockedBlueTeam,
                      PolicyService::actionFor(PolicyMode::BlueTeam,
                                               RiskClass::Offensive));
    TEST_ASSERT_EQUAL(PolicyAction::PromptValhalla,
                      PolicyService::actionFor(PolicyMode::Undecided,
                                               RiskClass::SensitiveData));
    TEST_ASSERT_EQUAL(PolicyAction::Proceed,
                      PolicyService::actionFor(PolicyMode::ValhallaAccepted,
                                               RiskClass::SensitiveData));
}

void test_legacy_mapping_is_fail_closed() {
    TEST_ASSERT_FALSE(PolicyService::legacyAllowsOffensive(false, false));
    TEST_ASSERT_FALSE(PolicyService::legacyAllowsOffensive(false, true));
    TEST_ASSERT_FALSE(PolicyService::legacyAllowsOffensive(true, true));
    TEST_ASSERT_TRUE(PolicyService::legacyAllowsOffensive(true, false));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_accept_valhalla_is_refused_while_in_blue_team);
    RUN_TEST(test_decline_then_accept_stays_in_blue_team);
    RUN_TEST(test_blue_team_blocks_without_offering_the_disclaimer);
    RUN_TEST(test_leave_blue_team_requires_an_acknowledgement);
    RUN_TEST(test_undecided_prompts_then_unlocks);
    RUN_TEST(test_passive_and_defensive_are_allowed_in_every_mode);
    RUN_TEST(test_enter_blue_team_preserves_the_disclaimer_bit);
    RUN_TEST(test_lockdown_clears_the_disclaimer_and_engages_blue_team);
    RUN_TEST(test_modeFor_collapses_two_bits_into_three_states);
    RUN_TEST(test_actionFor_is_a_pure_function_of_mode_and_risk);
    RUN_TEST(test_legacy_mapping_is_fail_closed);
    return UNITY_END();
}
