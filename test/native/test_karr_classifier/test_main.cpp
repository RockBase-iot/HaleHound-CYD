#include <unity.h>

#include "features/ble/KarrModel.h"

void test_framing_classification() {
    TEST_ASSERT_EQUAL(KarrFraming::Counter0xBB, KarrClassifier::classifyFraming(0xBB));
    TEST_ASSERT_EQUAL(KarrFraming::Encrypted0x55, KarrClassifier::classifyFraming(0x55));
    TEST_ASSERT_EQUAL(KarrFraming::Unsupported, KarrClassifier::classifyFraming(0x00));
    TEST_ASSERT_EQUAL(KarrFraming::Standard, KarrClassifier::classifyFraming(0x12));
    TEST_ASSERT_EQUAL_STRING("0xBB Counter", KarrClassifier::framingLabel(KarrFraming::Counter0xBB));
    TEST_ASSERT_EQUAL_STRING("0x55 Encrypted", KarrClassifier::framingLabel(KarrFraming::Encrypted0x55));
}

void test_assessment_never_commands_a_unit() {
    KarrEncounter encounter;
    encounter.framingByte = 0xBB;
    encounter.answered = true;
    encounter.replyStable = true;

    const KarrAssessment assessment = KarrClassifier::assess(encounter);
    TEST_ASSERT_FALSE(assessment.sendsCommands);
    TEST_ASSERT_FALSE(KarrClassifier::sendsCommands());
    // The raw encounter is always flagged for recording, whatever the verdict.
    TEST_ASSERT_TRUE(assessment.rawEncounterRecorded);
    TEST_ASSERT_EQUAL(KarrVerdict::Vulnerable, assessment.verdict);
    TEST_ASSERT_EQUAL_STRING("VULNERABLE", KarrClassifier::verdictLabel(assessment.verdict));
}

void test_unstable_standard_reply_is_patched_not_vulnerable() {
    KarrEncounter encounter;
    encounter.framingByte = 0x11;
    encounter.answered = true;
    encounter.replyStable = false;

    const KarrAssessment assessment = KarrClassifier::assess(encounter);
    TEST_ASSERT_EQUAL(KarrFraming::Standard, assessment.framing);
    TEST_ASSERT_EQUAL(KarrVerdict::Patched, assessment.verdict);
}

void test_encrypted_framing_is_never_called_vulnerable() {
    KarrEncounter stable;
    stable.framingByte = 0x55;
    stable.answered = true;
    stable.replyStable = true;
    const KarrAssessment patched = KarrClassifier::assess(stable);
    TEST_ASSERT_EQUAL(KarrVerdict::Patched, patched.verdict);

    KarrEncounter unstable = stable;
    unstable.replyStable = false;
    TEST_ASSERT_EQUAL(KarrVerdict::KeyRequired, KarrClassifier::assess(unstable).verdict);
}

void test_a_live_unit_with_unknown_framing_is_named_not_called_silent() {
    KarrEncounter encounter;
    encounter.framingByte = 0x00;
    encounter.answered = true;
    encounter.replyStable = true;

    const KarrAssessment assessment = KarrClassifier::assess(encounter);
    TEST_ASSERT_EQUAL(KarrFraming::Unsupported, assessment.framing);
    TEST_ASSERT_EQUAL(KarrVerdict::UnsupportedFraming, assessment.verdict);
    TEST_ASSERT_EQUAL_STRING("UNSUPPORTED FRAMING", KarrClassifier::verdictLabel(assessment.verdict));
    // The assessment still records the encounter so the unit is not lost.
    TEST_ASSERT_TRUE(assessment.rawEncounterRecorded);
    TEST_ASSERT_FALSE(assessment.sendsCommands);
}

void test_proximity_key_requirement_beats_a_stable_reply() {
    KarrEncounter encounter;
    encounter.framingByte = 0xBB;
    encounter.answered = true;
    encounter.replyStable = true;
    encounter.proximityKeyRequired = true;

    const KarrAssessment assessment = KarrClassifier::assess(encounter);
    TEST_ASSERT_EQUAL(KarrVerdict::KeyRequired, assessment.verdict);
    TEST_ASSERT_EQUAL_STRING("NEEDS KEY", KarrClassifier::verdictLabel(assessment.verdict));
}

void test_connect_failure_reports_a_reason_and_still_records() {
    KarrEncounter encounter;
    encounter.framingByte = 0xBB;
    encounter.failure = KarrConnectFailure::Timeout;

    const KarrAssessment assessment = KarrClassifier::assess(encounter);
    TEST_ASSERT_EQUAL(KarrVerdict::ConnectFailed, assessment.verdict);
    TEST_ASSERT_EQUAL_STRING("connect timed out", assessment.reason);
    TEST_ASSERT_TRUE(assessment.rawEncounterRecorded);

    encounter.failure = KarrConnectFailure::RadioBusy;
    TEST_ASSERT_EQUAL_STRING("radio stack busy", KarrClassifier::assess(encounter).reason);
    TEST_ASSERT_EQUAL_STRING("no failure", KarrClassifier::failureLabel(KarrConnectFailure::None));
}

void test_never_answered_is_a_connect_failure() {
    KarrEncounter encounter;
    encounter.framingByte = 0xBB;
    encounter.answered = false;
    const KarrAssessment assessment = KarrClassifier::assess(encounter);
    TEST_ASSERT_EQUAL(KarrVerdict::ConnectFailed, assessment.verdict);
    TEST_ASSERT_EQUAL_STRING("unit never answered", assessment.reason);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_framing_classification);
    RUN_TEST(test_assessment_never_commands_a_unit);
    RUN_TEST(test_unstable_standard_reply_is_patched_not_vulnerable);
    RUN_TEST(test_encrypted_framing_is_never_called_vulnerable);
    RUN_TEST(test_a_live_unit_with_unknown_framing_is_named_not_called_silent);
    RUN_TEST(test_proximity_key_requirement_beats_a_stable_reply);
    RUN_TEST(test_connect_failure_reports_a_reason_and_still_records);
    RUN_TEST(test_never_answered_is_a_connect_failure);
    return UNITY_END();
}
