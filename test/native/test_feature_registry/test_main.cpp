#include <unity.h>

#include "core/FeatureRegistry.h"
#include "features/FeatureCatalog.h"

void test_registry_exposes_stable_feature_descriptors() {
    const auto& registry = featureCatalog();
    const auto* deauth = registry.find(FeatureId::WifiDeauth);
    TEST_ASSERT_NOT_NULL(deauth);
    TEST_ASSERT_EQUAL(RiskClass::Offensive, deauth->risk);
    TEST_ASSERT_EQUAL(0, deauth->requiredPeripheralCount);

    const auto* subReplay = registry.find(FeatureId::SubGhzReplay);
    TEST_ASSERT_NOT_NULL(subReplay);
    TEST_ASSERT_EQUAL(PeripheralId::Cc1101, subReplay->requiredPeripherals[0]);
}

void test_registry_has_unique_ids() {
    const auto& registry = featureCatalog();
    for (size_t index = 0; index < registry.size(); ++index) {
        for (size_t other = index + 1; other < registry.size(); ++other) {
            TEST_ASSERT_NOT_EQUAL(
                static_cast<int>(registry.at(index).id),
                static_cast<int>(registry.at(other).id));
        }
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_exposes_stable_feature_descriptors);
    RUN_TEST(test_registry_has_unique_ids);
    return UNITY_END();
}
