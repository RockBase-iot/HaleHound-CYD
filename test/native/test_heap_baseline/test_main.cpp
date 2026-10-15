#include <unity.h>

#include <cstring>

#include "services/HeapBaseline.h"

namespace {

HeapBaseline baselineOf(const char* board, uint32_t free, uint32_t minFree,
                        uint32_t largest) {
    HeapBaseline baseline{};
    std::strncpy(baseline.board, board, sizeof(baseline.board) - 1);
    baseline.freeHeap = free;
    baseline.minFreeHeap = minFree;
    baseline.largestBlock = largest;
    return baseline;
}

}  // namespace

void test_an_identical_sample_is_within_budget() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 143216U, 118944U, 65536U);
    const HeapSample sample{143216U, 118944U, 65536U};

    HeapBaselineCheck check;
    const HeapAssessment result = check.assess(baseline, sample);
    TEST_ASSERT_EQUAL(HeapVerdict::WithinBudget, result.verdict);
    TEST_ASSERT_EQUAL(0, (int)result.freeDelta);
    TEST_ASSERT_EQUAL(0, (int)result.percentDeltaX100);
}

void test_a_nine_percent_regression_warns() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 100000U, 90000U, 40000U);
    const HeapSample sample{91000U, 82000U, 40000U};

    HeapAssessment result = HeapBaselineCheck{}.assess(baseline, sample);
    TEST_ASSERT_EQUAL(HeapVerdict::Warn, result.verdict);
    TEST_ASSERT_EQUAL(-9000, (int)result.freeDelta);
    TEST_ASSERT_EQUAL(900, (int)result.percentDeltaX100);
}

void test_a_sixteen_percent_regression_is_critical() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 100000U, 90000U, 40000U);
    const HeapSample sample{84000U, 80000U, 40000U};

    HeapAssessment result = HeapBaselineCheck{}.assess(baseline, sample);
    TEST_ASSERT_EQUAL(HeapVerdict::Critical, result.verdict);
    TEST_ASSERT_EQUAL(1600, (int)result.percentDeltaX100);
}

void test_an_improvement_never_warns() {
    const HeapBaseline baseline = baselineOf("esp32-cyd", 100000U, 90000U, 40000U);
    const HeapSample sample{120000U, 110000U, 60000U};

    HeapAssessment result = HeapBaselineCheck{}.assess(baseline, sample);
    TEST_ASSERT_EQUAL(HeapVerdict::WithinBudget, result.verdict);
    TEST_ASSERT_EQUAL(20000, (int)result.freeDelta);
    TEST_ASSERT_EQUAL(-2000, (int)result.percentDeltaX100);
}

// The absolute floor must beat a comfortable relative number: 25.6 KiB -> 24.0
// KiB is only 6.25 %, well under the 8 % warn threshold, but it is below the
// floor the TLS path needs.
void test_the_absolute_floor_overrides_a_small_percentage() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 25600U, 25000U, 16000U);
    const HeapSample sample{24000U, 23000U, 16000U};

    HeapAssessment result = HeapBaselineCheck{}.assess(baseline, sample);
    TEST_ASSERT_EQUAL(HeapVerdict::Critical, result.verdict);
    TEST_ASSERT_EQUAL(625, (int)result.percentDeltaX100);
    TEST_ASSERT_LESS_THAN((int)HeapBaselineCheck::kDefaultWarnX100,
                          (int)result.percentDeltaX100);
}

void test_fragmentation_warns_even_when_the_totals_look_healthy() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 100000U, 90000U, 65536U);
    // Same free heap, but the largest allocatable block collapsed.
    const HeapSample sample{100000U, 90000U, 8000U};

    HeapAssessment result = HeapBaselineCheck{}.assess(baseline, sample);
    TEST_ASSERT_EQUAL(HeapVerdict::Warn, result.verdict);
    TEST_ASSERT_EQUAL(0, (int)result.percentDeltaX100);
}

void test_a_zero_free_baseline_is_invalid() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 0U, 0U, 0U);
    const HeapSample sample{100000U, 90000U, 60000U};
    TEST_ASSERT_EQUAL(HeapVerdict::Invalid,
                      HeapBaselineCheck{}.assess(baseline, sample).verdict);
}

void test_a_baseline_without_a_board_tag_is_invalid() {
    HeapBaseline baseline{};
    baseline.freeHeap = 100000U;
    const HeapSample sample{100000U, 90000U, 60000U};
    TEST_ASSERT_EQUAL(HeapVerdict::Invalid,
                      HeapBaselineCheck{}.assess(baseline, sample).verdict);
}

void test_thresholds_are_configurable() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 100000U, 90000U, 60000U);
    const HeapSample sample{94000U, 90000U, 60000U};  // -6 %

    TEST_ASSERT_EQUAL(HeapVerdict::WithinBudget,
                      HeapBaselineCheck{}.assess(baseline, sample).verdict);
    HeapBaselineCheck strict(300U, 500U);
    TEST_ASSERT_EQUAL(HeapVerdict::Critical,
                      strict.assess(baseline, sample).verdict);
}

void test_regression_math_is_exact_and_sign_correct() {
    TEST_ASSERT_EQUAL(500, (int)HeapBaselineCheck::regressionX100(100000U, 95000U));
    TEST_ASSERT_EQUAL(-500, (int)HeapBaselineCheck::regressionX100(100000U, 105000U));
    TEST_ASSERT_EQUAL(0, (int)HeapBaselineCheck::regressionX100(100000U, 100000U));
    TEST_ASSERT_EQUAL(10000, (int)HeapBaselineCheck::regressionX100(100000U, 0U));
    // A zero reference cannot express a percentage; it must not divide.
    TEST_ASSERT_EQUAL(0, (int)HeapBaselineCheck::regressionX100(0U, 12345U));
}

void test_the_stored_line_round_trips() {
    const HeapBaseline baseline = baselineOf("nm-cyd-c5", 143216U, 118944U, 65536U);
    char line[160] = {0};
    const size_t written = formatHeapBaseline(baseline, line, sizeof(line));
    TEST_ASSERT_GREATER_THAN(0, (int)written);

    HeapBaseline parsed{};
    TEST_ASSERT_TRUE(parseHeapBaseline(line, parsed));
    TEST_ASSERT_EQUAL_STRING("nm-cyd-c5", parsed.board);
    TEST_ASSERT_EQUAL((int)baseline.freeHeap, (int)parsed.freeHeap);
    TEST_ASSERT_EQUAL((int)baseline.minFreeHeap, (int)parsed.minFreeHeap);
    TEST_ASSERT_EQUAL((int)baseline.largestBlock, (int)parsed.largestBlock);
}

void test_the_stored_line_is_byte_stable() {
    const HeapBaseline baseline = baselineOf("esp32-cyd", 121024U, 100000U, 40960U);
    char first[160] = {0};
    char second[160] = {0};
    formatHeapBaseline(baseline, first, sizeof(first));
    formatHeapBaseline(baseline, second, sizeof(second));
    TEST_ASSERT_EQUAL_STRING(first, second);
    TEST_ASSERT_EQUAL_STRING(
        "board=esp32-cyd free=121024 minFree=100000 largest=40960", first);
}

void test_a_truncated_field_set_is_rejected() {
    HeapBaseline parsed{};
    TEST_ASSERT_FALSE(parseHeapBaseline(nullptr, parsed));
    TEST_ASSERT_FALSE(parseHeapBaseline("", parsed));
    TEST_ASSERT_FALSE(parseHeapBaseline("board=nm-cyd-c5 free=1", parsed));
    TEST_ASSERT_FALSE(parseHeapBaseline("free=1 minFree=2 largest=3", parsed));
    TEST_ASSERT_FALSE(
        parseHeapBaseline("board=nm-cyd-c5 free=x minFree=2 largest=3", parsed));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_an_identical_sample_is_within_budget);
    RUN_TEST(test_a_nine_percent_regression_warns);
    RUN_TEST(test_a_sixteen_percent_regression_is_critical);
    RUN_TEST(test_an_improvement_never_warns);
    RUN_TEST(test_the_absolute_floor_overrides_a_small_percentage);
    RUN_TEST(test_fragmentation_warns_even_when_the_totals_look_healthy);
    RUN_TEST(test_a_zero_free_baseline_is_invalid);
    RUN_TEST(test_a_baseline_without_a_board_tag_is_invalid);
    RUN_TEST(test_thresholds_are_configurable);
    RUN_TEST(test_regression_math_is_exact_and_sign_correct);
    RUN_TEST(test_the_stored_line_round_trips);
    RUN_TEST(test_the_stored_line_is_byte_stable);
    RUN_TEST(test_a_truncated_field_set_is_rejected);
    return UNITY_END();
}
