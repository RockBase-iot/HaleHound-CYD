#include <unity.h>

#include <algorithm>

#include "services/FeatureSwitchLedger.h"

namespace {

// Profiles measured/estimated on the C5 (single core, shared FSPI). The point
// of the model is not the exact numbers but that the collapse is detectable.
const FeatureProfile kJammer{"ble-jammer", true, 180U, 40U, 120U, 0U};
const FeatureProfile kSpoofer{"ble-spoofer", true, 150U, 60U, 110U, 0U};
const FeatureProfile kAirTag{"airtag-hub", true, 200U, 80U, 140U, 0U};
const FeatureProfile kScanner{"ble-scanner", true, 90U, 30U, 70U, 0U};
const FeatureProfile kSkeleton{"skeleton-key", true, 160U, 50U, 130U, 0U};

// A feature that never leaves the BLE stack.
const FeatureProfile kStuck{"ble-stuck", true, 120U, 40U, 0U, 0U};
// A feature whose single blocking stretch blows past the render budget.
const FeatureProfile kBlocking{"ble-blocking", true, 100U, 900U, 90U, 0U};
// A feature that keeps a lease after teardown.
const FeatureProfile kLeaky{"ble-leaky", true, 100U, 40U, 90U, 1U};

SwitchIssue replay(const FeatureProfile* const* order, size_t count,
                   uint32_t budget = FeatureSwitchLedger::kDefaultRenderBudgetMs) {
    FeatureSwitchLedger ledger(budget);
    return ledger.replay(order, count);
}

}  // namespace

// ─── The requirement: any order must be clean ──────────────────────────────

void test_every_permutation_of_five_ble_features_is_clean() {
    const FeatureProfile* profiles[5] = {&kJammer, &kSpoofer, &kAirTag, &kScanner,
                                         &kSkeleton};
    const FeatureProfile* order[5];
    int perms = 0;
    do {
        for (int i = 0; i < 5; ++i) {
            order[i] = profiles[i];
        }
        const SwitchIssue issue = replay(order, 5);
        TEST_ASSERT_TRUE_MESSAGE(issue.clean(), issue.detail);
        ++perms;
    } while (std::next_permutation(profiles, profiles + 5));

    TEST_ASSERT_EQUAL(120, perms);
}

void test_every_permutation_of_four_ble_features_is_clean() {
    const FeatureProfile* profiles[4] = {&kJammer, &kSpoofer, &kScanner, &kSkeleton};
    const FeatureProfile* order[4];
    do {
        for (int i = 0; i < 4; ++i) {
            order[i] = profiles[i];
        }
        TEST_ASSERT_TRUE(replay(order, 4).clean());
    } while (std::next_permutation(profiles, profiles + 4));
}

// ─── White screen: one uninterrupted window past the budget ────────────────

void test_a_blocking_tick_is_reported_as_display_starvation() {
    const FeatureProfile* order[3] = {&kScanner, &kBlocking, &kJammer};
    const SwitchIssue issue = replay(order, 3);
    TEST_ASSERT_EQUAL(SwitchIssueCode::DisplayStarved, issue.code);
    TEST_ASSERT_EQUAL(900, (int)issue.observedMs);
    TEST_ASSERT_EQUAL_STRING("ble-blocking", issue.feature);
}

void test_a_long_begin_window_alone_starves_the_display() {
    const FeatureProfile slowBegin{"ble-slow-init", true, 420U, 20U, 60U, 0U};
    const FeatureProfile* order[1] = {&slowBegin};
    const SwitchIssue issue = replay(order, 1);
    TEST_ASSERT_EQUAL(SwitchIssueCode::DisplayStarved, issue.code);
    TEST_ASSERT_EQUAL(420, (int)issue.observedMs);
}

// A tighter budget must reject a switch that the default budget accepts —
// i.e. the budget is a knob, not a constant baked into the check.
void test_the_render_budget_is_honoured() {
    const FeatureProfile* order[1] = {&kJammer};  // worst window 180 ms
    TEST_ASSERT_TRUE(replay(order, 1, 250U).clean());
    TEST_ASSERT_EQUAL(SwitchIssueCode::DisplayStarved,
                      replay(order, 1, 150U).code);
}

void test_worst_window_reports_the_metric_for_a_clean_order() {
    const FeatureProfile* order[3] = {&kScanner, &kJammer, &kSkeleton};
    FeatureSwitchLedger ledger;
    TEST_ASSERT_EQUAL(180, (int)ledger.worstWindowMs(order, 3));
}

// ─── Stack hand-off and resource hygiene ───────────────────────────────────

void test_a_ble_feature_without_teardown_breaks_the_next_handover() {
    const FeatureProfile* order[3] = {&kScanner, &kStuck, &kJammer};
    const SwitchIssue issue = replay(order, 3);
    TEST_ASSERT_EQUAL(SwitchIssueCode::TeardownMissing, issue.code);
    // The issue is attributed to the feature being entered, not the stuck one.
    TEST_ASSERT_EQUAL_STRING("ble-jammer", issue.feature);
}

void test_a_stuck_feature_last_in_the_order_is_not_a_handover_violation() {
    const FeatureProfile* order[2] = {&kScanner, &kStuck};
    TEST_ASSERT_TRUE(replay(order, 2).clean());
}

void test_switching_between_non_ble_features_ignores_ble_teardown() {
    const FeatureProfile passive{"subghz-read", false, 60U, 40U, 0U, 0U};
    const FeatureProfile passive2{"ir-blast", false, 50U, 30U, 0U, 0U};
    const FeatureProfile* order[2] = {&passive, &passive2};
    TEST_ASSERT_TRUE(replay(order, 2).clean());
}

void test_a_surviving_lease_is_reported_as_a_resource_leak() {
    const FeatureProfile* order[2] = {&kScanner, &kLeaky};
    const SwitchIssue issue = replay(order, 2);
    TEST_ASSERT_EQUAL(SwitchIssueCode::ResourceLeak, issue.code);
    TEST_ASSERT_EQUAL_STRING("ble-leaky", issue.feature);
    TEST_ASSERT_EQUAL(1, (int)issue.observedMs);
}

// ─── Contract details worth pinning ────────────────────────────────────────

void test_time_starvation_is_checked_before_the_handover() {
    // kStuck never tears down AND kBlocking exceeds the budget: the time
    // verdict wins, and that ordering is part of the contract.
    const FeatureProfile* order[2] = {&kStuck, &kBlocking};
    TEST_ASSERT_EQUAL(SwitchIssueCode::DisplayStarved, replay(order, 2).code);
}

void test_degenerate_orders_are_rejected_not_ignored() {
    TEST_ASSERT_EQUAL(SwitchIssueCode::InvalidProfile, replay(nullptr, 3).code);

    const FeatureProfile* withNull[2] = {&kScanner, nullptr};
    TEST_ASSERT_EQUAL(SwitchIssueCode::InvalidProfile, replay(withNull, 2).code);

    const FeatureProfile noId{"", true, 10U, 10U, 10U, 0U};
    const FeatureProfile* emptyId[1] = {&noId};
    TEST_ASSERT_EQUAL(SwitchIssueCode::InvalidProfile, replay(emptyId, 1).code);
}

void test_a_zero_length_order_is_clean() {
    TEST_ASSERT_TRUE(replay(nullptr, 0).clean());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_permutation_of_five_ble_features_is_clean);
    RUN_TEST(test_every_permutation_of_four_ble_features_is_clean);
    RUN_TEST(test_a_blocking_tick_is_reported_as_display_starvation);
    RUN_TEST(test_a_long_begin_window_alone_starves_the_display);
    RUN_TEST(test_the_render_budget_is_honoured);
    RUN_TEST(test_worst_window_reports_the_metric_for_a_clean_order);
    RUN_TEST(test_a_ble_feature_without_teardown_breaks_the_next_handover);
    RUN_TEST(test_a_stuck_feature_last_in_the_order_is_not_a_handover_violation);
    RUN_TEST(test_switching_between_non_ble_features_ignores_ble_teardown);
    RUN_TEST(test_a_surviving_lease_is_reported_as_a_resource_leak);
    RUN_TEST(test_time_starvation_is_checked_before_the_handover);
    RUN_TEST(test_degenerate_orders_are_rejected_not_ignored);
    RUN_TEST(test_a_zero_length_order_is_clean);
    return UNITY_END();
}
