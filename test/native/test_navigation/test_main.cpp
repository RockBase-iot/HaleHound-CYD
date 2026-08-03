#include <unity.h>

#include "core/NavigationController.h"
#include "features/LegacyFeatureAdapter.h"

namespace {
int setupCount = 0;
int loopCount = 0;
int cleanupCount = 0;

void setupCallback() { ++setupCount; }
void loopCallback() { ++loopCount; }
bool exitCallback() { return false; }
void cleanupCallback() { ++cleanupCount; }

void test_navigation_keeps_blocked_feature_and_returns_on_completion() {
    NavigationController navigation;
    TEST_ASSERT_EQUAL(NavigationOutcome::Pushed,
                      navigation.push(FeatureId::WifiScanner));
    TEST_ASSERT_EQUAL(NavigationOutcome::Pushed,
                      navigation.push(FeatureId::WifiDeauth));
    TEST_ASSERT_EQUAL(NavigationOutcome::Blocked,
                      navigation.complete({FeatureResultCode::Blocked, "policy"}));
    TEST_ASSERT_EQUAL(FeatureId::WifiDeauth, navigation.current());
    TEST_ASSERT_EQUAL(NavigationOutcome::Returned,
                      navigation.complete({FeatureResultCode::Completed, "done"}));
    TEST_ASSERT_EQUAL(FeatureId::WifiScanner, navigation.current());
}

void test_navigation_fault_is_visible_and_back_is_explicit() {
    NavigationController navigation;
    navigation.push(FeatureId::WifiScanner);
    navigation.push(FeatureId::WifiDeauth);
    TEST_ASSERT_EQUAL(NavigationOutcome::Fault,
                      navigation.complete({FeatureResultCode::Fault, "radio"}));
    TEST_ASSERT_EQUAL_STRING("radio", navigation.lastError());
    TEST_ASSERT_EQUAL(NavigationOutcome::Returned, navigation.back());
    TEST_ASSERT_EQUAL(FeatureId::WifiScanner, navigation.current());
}

void test_legacy_adapter_calls_callbacks_once_per_lifecycle_step() {
    setupCount = 0;
    loopCount = 0;
    cleanupCount = 0;
    LegacyFeatureAdapter adapter({setupCallback, loopCallback, exitCallback, cleanupCallback});
    FeatureContext context{};
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, adapter.begin(context).code);
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, adapter.begin(context).code);
    adapter.tick();
    adapter.tick();
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, adapter.end().code);
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, adapter.end().code);
    TEST_ASSERT_EQUAL(1, setupCount);
    TEST_ASSERT_EQUAL(2, loopCount);
    TEST_ASSERT_EQUAL(1, cleanupCount);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_navigation_keeps_blocked_feature_and_returns_on_completion);
    RUN_TEST(test_navigation_fault_is_visible_and_back_is_explicit);
    RUN_TEST(test_legacy_adapter_calls_callbacks_once_per_lifecycle_step);
    return UNITY_END();
}
