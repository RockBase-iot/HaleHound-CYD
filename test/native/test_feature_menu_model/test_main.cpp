#include <unity.h>

#include <cstring>

#include "core/FeatureRegistry.h"
#include "features/FeatureMenuModel.h"

// The regression these exist for: the legacy Bluetooth submenu was a hand-kept
// array of ten names beside a hand-written switch, and the catalog was a third
// list. Bluetooth had fourteen catalog entries and nine menu rows, so five
// working screens were unreachable from the device -- and nothing failed,
// because nothing compared the two.
namespace {

const CategoryId kAllCategories[] = {
    CategoryId::Wifi,    CategoryId::Bluetooth, CategoryId::Nrf24,
    CategoryId::SubGhz,  CategoryId::Rfid,      CategoryId::Sigint,
    CategoryId::Defensive, CategoryId::Tools,   CategoryId::Settings,
    CategoryId::Ir,      CategoryId::Rf433,
};

// Counts the category straight off the registry, so the assertion does not
// depend on the thing under test.
uint8_t registryCount(CategoryId category) {
    const FeatureRegistry& registry = featureCatalog();
    uint8_t found = 0;
    for (size_t index = 0; index < registry.size(); ++index) {
        if (registry.at(index).category == category) {
            ++found;
        }
    }
    return found;
}

bool containsId(const FeatureMenuItem* items, uint8_t count, FeatureId id) {
    for (uint8_t index = 0; index < count; ++index) {
        if (items[index].id == id) {
            return true;
        }
    }
    return false;
}

}  // namespace

void test_count_matches_the_registry_for_every_category() {
    for (CategoryId category : kAllCategories) {
        TEST_ASSERT_EQUAL(registryCount(category), FeatureMenuModel::count(category));
    }
}

void test_build_writes_the_same_rows_the_count_promises() {
    for (CategoryId category : kAllCategories) {
        FeatureMenuItem items[32]{};
        const uint8_t written =
            FeatureMenuModel::build(category, items, sizeof(items) / sizeof(items[0]));
        TEST_ASSERT_EQUAL(FeatureMenuModel::count(category), written);
    }
}

// Every row of every category carries a label and a runnable factory. A menu row
// that cannot start anything is worse than a missing row: the button looks like
// it works.
void test_every_row_is_labelled_and_runnable() {
    const FeatureRegistry& registry = featureCatalog();
    for (CategoryId category : kAllCategories) {
        FeatureMenuItem items[32]{};
        const uint8_t count =
            FeatureMenuModel::build(category, items, sizeof(items) / sizeof(items[0]));
        for (uint8_t index = 0; index < count; ++index) {
            TEST_ASSERT_NOT_NULL(items[index].label);
            TEST_ASSERT_TRUE(std::strlen(items[index].label) > 0);
            const FeatureDescriptor* descriptor = registry.find(items[index].id);
            TEST_ASSERT_NOT_NULL(descriptor);
            TEST_ASSERT_NOT_NULL(descriptor->factory);
            // And the label is the catalog's own string, not a local copy that
            // could drift from it again.
            TEST_ASSERT_EQUAL_PTR(descriptor->displayName, items[index].label);
        }
    }
}

// The five entries the hardcoded Bluetooth array never had. Pinned by name
// because "the menu shows everything the catalog has" is the property, and these
// are the specific rows whose absence was the symptom.
void test_the_bluetooth_menu_no_longer_drops_the_new_screens() {
    FeatureMenuItem items[32]{};
    const uint8_t count = FeatureMenuModel::build(
        CategoryId::Bluetooth, items, sizeof(items) / sizeof(items[0]));

    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothSkeletonKey));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothHidReader));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothKarr));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothAirohaRace));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothPredator));

    // The nine the old array did have are all still there, so the fix added rows
    // rather than swapping the set.
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothJammer));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothSpoofer));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothBeacon));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothSniffer));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothScanner));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothWhisperPair));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothAirTag));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothLunaticFringe));
    TEST_ASSERT_TRUE(containsId(items, count, FeatureId::BluetoothDucky));
}

// Catalog order, not sorted and not arbitrary: the menu's row order has to be a
// function of the table, or the same catalog produces two menus.
void test_rows_follow_catalog_order() {
    FeatureMenuItem items[32]{};
    const uint8_t count = FeatureMenuModel::build(
        CategoryId::Bluetooth, items, sizeof(items) / sizeof(items[0]));

    const FeatureRegistry& registry = featureCatalog();
    uint8_t cursor = 0;
    for (size_t index = 0; index < registry.size(); ++index) {
        if (registry.at(index).category != CategoryId::Bluetooth) {
            continue;
        }
        TEST_ASSERT_TRUE(cursor < count);
        TEST_ASSERT_EQUAL(registry.at(index).id, items[cursor].id);
        ++cursor;
    }
    TEST_ASSERT_EQUAL(count, cursor);
}

void test_capacity_is_respected_rather_than_overflowed() {
    FeatureMenuItem items[2]{};
    items[0].id = FeatureId::WifiDeauth;
    items[1].id = FeatureId::WifiDeauth;

    TEST_ASSERT_TRUE(FeatureMenuModel::count(CategoryId::Bluetooth) > 2);
    const uint8_t written = FeatureMenuModel::build(CategoryId::Bluetooth, items, 2);

    // A short count is the report. Overwriting past the array would be the bug
    // this check exists to keep out.
    TEST_ASSERT_EQUAL(2, written);
}

void test_a_null_or_zero_capacity_build_writes_nothing() {
    FeatureMenuItem items[4]{};
    TEST_ASSERT_EQUAL(0, FeatureMenuModel::build(CategoryId::Bluetooth, nullptr, 4));
    TEST_ASSERT_EQUAL(0, FeatureMenuModel::build(CategoryId::Bluetooth, items, 0));
}

// The legacy menu needs an upper bound to size its arrays with. If a category
// ever outgrows it, this fails here rather than as a silently short menu on a
// device.
void test_the_legacy_menu_buffer_still_fits_every_category() {
    // Must match kBluetoothMenuMax - 1 in LegacyApp.inc: the rows the model may
    // write, with the Back row taking the last slot.
    constexpr uint8_t kLegacyBluetoothMenuRows = 23;
    TEST_ASSERT_TRUE(FeatureMenuModel::count(CategoryId::Bluetooth) <=
                     kLegacyBluetoothMenuRows);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_count_matches_the_registry_for_every_category);
    RUN_TEST(test_build_writes_the_same_rows_the_count_promises);
    RUN_TEST(test_every_row_is_labelled_and_runnable);
    RUN_TEST(test_the_bluetooth_menu_no_longer_drops_the_new_screens);
    RUN_TEST(test_rows_follow_catalog_order);
    RUN_TEST(test_capacity_is_respected_rather_than_overflowed);
    RUN_TEST(test_a_null_or_zero_capacity_build_writes_nothing);
    RUN_TEST(test_the_legacy_menu_buffer_still_fits_every_category);
    return UNITY_END();
}
