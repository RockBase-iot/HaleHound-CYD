#include <unity.h>

#include "core/FeatureTypes.h"

void test_offensive_feature_is_not_passive() {
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(RiskClass::Passive),
        static_cast<int>(RiskClass::Offensive));
}

void test_not_found_result_preserves_code() {
    const FeatureResult result{FeatureResultCode::NotFound, "cc1101"};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(FeatureResultCode::NotFound),
        static_cast<int>(result.code));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_offensive_feature_is_not_passive);
    RUN_TEST(test_not_found_result_preserves_code);
    return UNITY_END();
}
