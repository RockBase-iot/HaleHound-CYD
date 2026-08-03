#include <cstring>
#include <unity.h>

#include "features/FeatureCatalog.h"
#include "core/FeatureRunner.h"

namespace {
size_t countCategory(CategoryId category) {
    size_t count = 0;
    const auto& catalog = featureCatalog();
    for (size_t index = 0; index < catalog.size(); ++index) {
        if (catalog.at(index).category == category) {
            ++count;
        }
    }
    return count;
}

void test_catalog_has_complete_menu_categories() {
    TEST_ASSERT_EQUAL(8, countCategory(CategoryId::Wifi));
    TEST_ASSERT_TRUE(countCategory(CategoryId::Bluetooth) >= 9);
    TEST_ASSERT_EQUAL(6, countCategory(CategoryId::Nrf24));
    TEST_ASSERT_TRUE(countCategory(CategoryId::SubGhz) >= 5);
    TEST_ASSERT_TRUE(countCategory(CategoryId::Rfid) >= 5);
    TEST_ASSERT_EQUAL(4, countCategory(CategoryId::Defensive));
    TEST_ASSERT_TRUE(countCategory(CategoryId::Sigint) >= 6);
    TEST_ASSERT_EQUAL(5, countCategory(CategoryId::Tools));
    TEST_ASSERT_EQUAL(9, countCategory(CategoryId::Settings));
}

void test_every_catalog_item_has_factory_and_lifecycle() {
    const auto& catalog = featureCatalog();
    FeatureContext context{};
    for (size_t index = 0; index < catalog.size(); ++index) {
        const auto& descriptor = catalog.at(index);
        TEST_ASSERT_NOT_NULL(descriptor.factory);
        IFeature* feature = descriptor.factory();
        TEST_ASSERT_NOT_NULL(feature);
        const auto beginCode = feature->begin(context).code;
        TEST_ASSERT_TRUE(beginCode == FeatureResultCode::Completed ||
                         beginCode == FeatureResultCode::Blocked ||
                         beginCode == FeatureResultCode::Unsupported);
        feature->tick();
        feature->requestStop(StopReason::UserRequest);
        TEST_ASSERT_EQUAL(FeatureResultCode::Completed, feature->end().code);
        delete feature;
    }
}

void test_sensitive_entries_keep_contract_metadata() {
    const auto& catalog = featureCatalog();
    const auto* replay = catalog.find(FeatureId::SubGhzReplay);
    TEST_ASSERT_NOT_NULL(replay);
    TEST_ASSERT_EQUAL(RiskClass::Offensive, replay->risk);
    TEST_ASSERT_EQUAL(PeripheralId::Cc1101, replay->requiredPeripherals[0]);
    TEST_ASSERT_EQUAL_STRING("/subghz", replay->storagePath);

    const auto* ntag = catalog.find(FeatureId::NtagTools);
    TEST_ASSERT_NOT_NULL(ntag);
    TEST_ASSERT_EQUAL(PeripheralId::Pn532, ntag->requiredPeripherals[0]);
    TEST_ASSERT_EQUAL_STRING("/loot", ntag->storagePath);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_catalog_has_complete_menu_categories);
    RUN_TEST(test_every_catalog_item_has_factory_and_lifecycle);
    RUN_TEST(test_sensitive_entries_keep_contract_metadata);
    return UNITY_END();
}
