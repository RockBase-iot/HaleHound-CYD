#include <unity.h>

#include "features/skimmer/SkimmerScore.h"

void test_skimmer_score_explains_evidence_and_threshold() {
    SkimmerEvidence evidence;
    evidence.nameMatch = true;
    evidence.serviceMatch = true;
    evidence.behaviorMatch = false;
    evidence.rssi = -48;
    const auto score = SkimmerScorer::evaluate(evidence);
    TEST_ASSERT_TRUE(score.likelySkimmer);
    TEST_ASSERT_TRUE(score.value >= 70);
    TEST_ASSERT_GREATER_THAN(1, score.reasons.size());
}

void test_skimmer_active_confirmation_is_explicit() {
    SkimmerHuntSession session;
    TEST_ASSERT_FALSE(session.active());
    TEST_ASSERT_FALSE(session.start(false));
    TEST_ASSERT_TRUE(session.start(true));
    TEST_ASSERT_TRUE(session.active());
    session.stop();
    TEST_ASSERT_FALSE(session.active());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_skimmer_score_explains_evidence_and_threshold);
    RUN_TEST(test_skimmer_active_confirmation_is_explicit);
    return UNITY_END();
}
