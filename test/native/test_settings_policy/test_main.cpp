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
}

void test_missing_settings_default_to_blue_team() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);

    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, policy.mode());
}

void test_valid_valhalla_record_survives_reload() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);

    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(policy.acceptValhalla());

    SettingsService reloaded(backend);
    PolicyService reloadedPolicy(reloaded);
    TEST_ASSERT_TRUE(reloaded.load());
    TEST_ASSERT_EQUAL(PolicyMode::ValhallaAccepted, reloadedPolicy.mode());
}

void test_bad_crc_and_decline_keep_blue_team() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(policy.acceptValhalla());

    backend.records[0].crc ^= 0x01U;
    backend.records[1].crc ^= 0x01U;
    SettingsService corrupted(backend);
    PolicyService corruptedPolicy(corrupted);
    TEST_ASSERT_TRUE(corrupted.load());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, corruptedPolicy.mode());
    TEST_ASSERT_TRUE(corruptedPolicy.enterBlueTeam());
    TEST_ASSERT_EQUAL(PolicyMode::BlueTeam, corruptedPolicy.mode());
}

void test_blue_team_blocks_offensive_and_allows_defensive() {
    MemoryBackend backend;
    SettingsService settings(backend);
    PolicyService policy(settings);
    TEST_ASSERT_TRUE(settings.load());

    const FeatureDescriptor passive{
        FeatureId::WifiScanner, CategoryId::Wifi, "scan", RiskClass::Passive,
        {}, 0, {}, 0, "", "", nullptr};
    const FeatureDescriptor offensive{
        FeatureId::WifiDeauth, CategoryId::Wifi, "deauth", RiskClass::Offensive,
        {}, 0, {}, 0, "", "", nullptr};
    TEST_ASSERT_TRUE(policy.authorize(passive).allowed());
    TEST_ASSERT_FALSE(policy.authorize(offensive).allowed());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_missing_settings_default_to_blue_team);
    RUN_TEST(test_valid_valhalla_record_survives_reload);
    RUN_TEST(test_bad_crc_and_decline_keep_blue_team);
    RUN_TEST(test_blue_team_blocks_offensive_and_allows_defensive);
    return UNITY_END();
}
