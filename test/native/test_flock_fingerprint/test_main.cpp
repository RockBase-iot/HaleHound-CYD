#include <unity.h>

#include "features/sigint/FlockFingerprint.h"

void test_flock_known_oui_and_raven_service_score() {
    FlockObservation observation;
    observation.oui = "00:11:22";
    observation.serviceUuids = {FlockFingerprint::ravenServiceUuid()};
    observation.gpsValid = true;
    observation.latitude = 31.2;
    observation.longitude = 121.5;
    const auto result = FlockFingerprint::evaluate(observation);
    TEST_ASSERT_TRUE(result.matched);
    TEST_ASSERT_TRUE(result.confidence >= 80);
    TEST_ASSERT_TRUE(result.gpsTagged);
    TEST_ASSERT_EQUAL_STRING("/loot/flock", result.lootPath);
}

void test_flock_unknown_or_conflicting_observation_is_explainable() {
    FlockObservation observation;
    observation.oui = "DE:AD:BE";
    observation.serviceUuids = {"0000ffff-0000-1000-8000-00805f9b34fb"};
    const auto result = FlockFingerprint::evaluate(observation);
    TEST_ASSERT_FALSE(result.matched);
    TEST_ASSERT_TRUE(result.confidence < 80);
    TEST_ASSERT_FALSE(result.gpsTagged);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_flock_known_oui_and_raven_service_score);
    RUN_TEST(test_flock_unknown_or_conflicting_observation_is_explainable);
    return UNITY_END();
}
