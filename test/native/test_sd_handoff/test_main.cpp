#include <unity.h>

#include "services/SdHandoff.h"

// Bench regression: "radio feature -> back out -> open Loot -> card reads as
// empty". The volume was never re-mounted after the radio released the shared
// SPI bus, and the reader reported an empty directory instead of a state.

void test_read_without_a_mount_remounts_once() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdState::Unmounted, sd.state());

    const SdHandoffResult result = sd.readForLoot("/loot");
    TEST_ASSERT_EQUAL(SdCode::Remounted, result.code);
    TEST_ASSERT_EQUAL(1, (int)sd.remountCount());
    TEST_ASSERT_EQUAL(1, (int)sd.mountCount());
    TEST_ASSERT_EQUAL(SdState::Mounted, sd.state());
}

// The exact bench sequence for the SD item.
void test_radio_session_then_loot_read_remounts_and_succeeds() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdCode::Ready, sd.mount());

    TEST_ASSERT_EQUAL(SdCode::Ready, sd.beginRadioSession());
    TEST_ASSERT_EQUAL(SdState::RadioOwnsBus, sd.state());

    // Still inside the radio session: must NOT look like an empty card.
    const SdHandoffResult blocked = sd.readForLoot("/loot");
    TEST_ASSERT_EQUAL(SdCode::Busy, blocked.code);
    TEST_ASSERT_NOT_NULL(blocked.detail);
    TEST_ASSERT_EQUAL(1, (int)sd.blockedReadCount());

    TEST_ASSERT_EQUAL(SdCode::Ready, sd.endRadioSession());
    TEST_ASSERT_EQUAL(SdState::Unmounted, sd.state());

    // After teardown the reader re-mounts and only then reports success.
    const SdHandoffResult after = sd.readForLoot("/loot");
    TEST_ASSERT_EQUAL(SdCode::Remounted, after.code);
    TEST_ASSERT_EQUAL(SdState::Mounted, sd.state());
    TEST_ASSERT_EQUAL(1, (int)sd.remountCount());

    // A second read costs nothing.
    TEST_ASSERT_EQUAL(SdCode::Ready, sd.readForLoot("/loot").code);
    TEST_ASSERT_EQUAL(1, (int)sd.remountCount());
}

void test_a_blocked_read_is_never_reported_as_ready() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdCode::Ready, sd.beginRadioSession());
    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_EQUAL(SdCode::Busy, sd.readForLoot("/loot/creds.txt").code);
    }
    TEST_ASSERT_EQUAL(5, (int)sd.blockedReadCount());
}

void test_missing_card_reports_not_mounted_rather_than_empty() {
    SdHandoff sd(false);
    const SdHandoffResult result = sd.readForLoot("/loot");
    TEST_ASSERT_EQUAL(SdCode::NotMounted, result.code);
    TEST_ASSERT_EQUAL(1, (int)sd.blockedReadCount());
    TEST_ASSERT_EQUAL(SdState::Unmounted, sd.state());
}

void test_invalid_paths_are_rejected_before_touching_the_bus() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdCode::Invalid, sd.readForLoot(nullptr).code);
    TEST_ASSERT_EQUAL(SdCode::Invalid, sd.readForLoot("loot").code);
    TEST_ASSERT_EQUAL(SdCode::Invalid, sd.readForLoot("").code);
    // Rejected requests must not be counted as blocked reads.
    TEST_ASSERT_EQUAL(0, (int)sd.blockedReadCount());
}

void test_mount_and_unmount_are_refused_while_the_radio_owns_the_bus() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdCode::Ready, sd.beginRadioSession());

    TEST_ASSERT_EQUAL(SdCode::Busy, sd.mount());
    TEST_ASSERT_EQUAL(SdCode::Busy, sd.unmount());
    TEST_ASSERT_EQUAL(SdState::RadioOwnsBus, sd.state());
    TEST_ASSERT_EQUAL(0, (int)sd.mountCount());
}

void test_radio_session_pairing_is_enforced() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdCode::Invalid, sd.endRadioSession());
    TEST_ASSERT_EQUAL(SdCode::Ready, sd.beginRadioSession());
    TEST_ASSERT_EQUAL(SdCode::Invalid, sd.beginRadioSession());
}

void test_unmounting_an_unmounted_card_is_harmless() {
    SdHandoff sd(true);
    TEST_ASSERT_EQUAL(SdCode::Ready, sd.unmount());
    TEST_ASSERT_EQUAL(SdState::Unmounted, sd.state());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_read_without_a_mount_remounts_once);
    RUN_TEST(test_radio_session_then_loot_read_remounts_and_succeeds);
    RUN_TEST(test_a_blocked_read_is_never_reported_as_ready);
    RUN_TEST(test_missing_card_reports_not_mounted_rather_than_empty);
    RUN_TEST(test_invalid_paths_are_rejected_before_touching_the_bus);
    RUN_TEST(test_mount_and_unmount_are_refused_while_the_radio_owns_the_bus);
    RUN_TEST(test_radio_session_pairing_is_enforced);
    RUN_TEST(test_unmounting_an_unmounted_card_is_harmless);
    return UNITY_END();
}
