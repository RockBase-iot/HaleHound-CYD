#include <unity.h>

#include "core/FeatureRunner.h"
#include "features/ble/BlePredatorModel.h"

void test_predator_phases_gate_honeypot_and_record_writes() {
    BlePredatorModel model;
    TEST_ASSERT_EQUAL(BlePredatorPhase::Idle, model.phase());
    TEST_ASSERT_TRUE(model.advance(BlePredatorPhase::Scan));
    TEST_ASSERT_TRUE(model.advance(BlePredatorPhase::Recon));
    TEST_ASSERT_TRUE(model.advance(BlePredatorPhase::Honeypot));
    TEST_ASSERT_TRUE(model.recordWrite("AA:BB:CC:DD:EE:FF", "svc", "value"));
    TEST_ASSERT_EQUAL(1, model.writeEvents().size());
    TEST_ASSERT_EQUAL_STRING("/wp_loot", model.lootPath());
    model.releasePhase();
    TEST_ASSERT_EQUAL(BlePredatorPhase::Idle, model.phase());
    TEST_ASSERT_FALSE(model.recordWrite("AA:BB:CC:DD:EE:FF", "svc", "late"));
}

void test_airoha_factory_is_evidence_gated() {
    IFeature* feature = createAirohaRaceFeature();
    TEST_ASSERT_NOT_NULL(feature);
    FeatureContext context{};
    TEST_ASSERT_EQUAL(FeatureResultCode::Unsupported, feature->begin(context).code);
    delete feature;
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_predator_phases_gate_honeypot_and_record_writes);
    RUN_TEST(test_airoha_factory_is_evidence_gated);
    return UNITY_END();
}
