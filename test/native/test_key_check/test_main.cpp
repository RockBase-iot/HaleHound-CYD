// Key Check reporting contract.
//
// The loop that walks the key table is trivial and the driver owns it; what this
// suite pins is the *reporting*, because that is where a plausible-looking lie
// can enter. Three specific lies it must be impossible to tell:
//
//   1. "N of M sectors recovered" counting keys tried rather than sectors opened.
//   2. A progress bar derived from sectors, which reads 0 % for the first ~58
//      attempts and then jumps -- indistinguishable from a hung scan.
//   3. "Finished" being rendered as "opened".

#include <unity.h>

#include <string>

#include "features/rfid/KeyCheckModel.h"

namespace {

// Drive the whole table at one slot and report whether every key was rejected.
// Kept as a helper so the tests read as "exhaust the sector" rather than as a
// 29-iteration loop.
void exhaustCurrentSector(KeyCheckSession& session, MifareKeySlot slot) {
    for (uint16_t index = 0; index < MifareKeyTable::count(); ++index) {
        session.onAttempt(slot, index, false);
    }
    session.onSectorExhausted();
}

}  // namespace

// ---------------------------------------------------------------------------
// Budget and ordering hints
// ---------------------------------------------------------------------------

void test_max_attempts_covers_both_slots_against_the_whole_table() {
    // 16 sectors x 2 slots x 29 published keys. This is the denominator the
    // progress bar divides by, so it has to be the honest worst case.
    TEST_ASSERT_EQUAL(928u, KeyCheckSession::maxAttempts(16));
    TEST_ASSERT_EQUAL(58u, KeyCheckSession::maxAttempts(1));
    TEST_ASSERT_EQUAL(2320u, KeyCheckSession::maxAttempts(kKeyCheckMaxSectors));
    TEST_ASSERT_EQUAL(0u, KeyCheckSession::maxAttempts(0));
}

void test_preferred_key_is_the_nfc_forum_key_on_ndef_cards() {
    // Sector 0 of an NDEF card is the MAD sector (key A = A0A1A2A3A4A5, table
    // index 2); the NDEF data sectors use D3F7D3F7D3F7 (index 23).
    TEST_ASSERT_EQUAL(2, KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyA, 0));
    TEST_ASSERT_EQUAL(23, KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyA, 1));
    TEST_ASSERT_EQUAL(23, KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyA, 15));
    // Key B has no NDEF convention at all -- falling back to a made-up "B"
    // counterpart would be a guess, so it points at the transport key.
    TEST_ASSERT_EQUAL(0, KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyB, 0));
    TEST_ASSERT_EQUAL(0, KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyB, 15));

    // The hint must actually be those keys, not two magic indices that happen to
    // be right today.
    const std::array<uint8_t, kMifareKeySize>& mad =
        MifareKeyTable::at(KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyA, 0)).bytes;
    TEST_ASSERT_EQUAL_STRING("A0A1A2A3A4A5", MifareKeyTable::toString(mad).c_str());
    const std::array<uint8_t, kMifareKeySize>& ndef =
        MifareKeyTable::at(KeyCheckSession::preferredKeyIndex(MifareKeySlot::KeyA, 4)).bytes;
    TEST_ASSERT_EQUAL_STRING("D3F7D3F7D3F7", MifareKeyTable::toString(ndef).c_str());
}

// ---------------------------------------------------------------------------
// Session lifecycle
// ---------------------------------------------------------------------------

void test_begin_refuses_a_card_size_it_cannot_represent() {
    KeyCheckSession session;
    TEST_ASSERT_FALSE(session.begin(0));
    TEST_ASSERT_FALSE(session.begin(kKeyCheckMaxSectors + 1));
    // A refused begin must leave the session idle -- not half-started with a
    // bogus denominator.
    TEST_ASSERT_EQUAL(KeyCheckState::Idle, session.report().state);
    TEST_ASSERT_EQUAL(0u, session.report().maxAttempts);

    TEST_ASSERT_TRUE(session.begin(kKeyCheckMaxSectors));
    TEST_ASSERT_EQUAL(KeyCheckState::Running, session.report().state);
    TEST_ASSERT_EQUAL(kKeyCheckMaxSectors, session.report().sectorsTotal);
    TEST_ASSERT_EQUAL(0u, session.report().attempts);
    TEST_ASSERT_EQUAL(0, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(0, session.currentSector());
}

void test_a_sector_is_recovered_only_when_authentication_succeeds() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(3));

    // 29 rejected keys must leave the counter at zero and the scan on sector 0.
    for (uint16_t index = 0; index < MifareKeyTable::count(); ++index) {
        session.onAttempt(MifareKeySlot::KeyA, index, false);
    }
    TEST_ASSERT_EQUAL(0, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(0, session.currentSector());
    TEST_ASSERT_EQUAL(29u, session.report().attempts);

    session.onAttempt(MifareKeySlot::KeyB, 0, true);
    TEST_ASSERT_EQUAL(1, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(1, session.currentSector());

    // The key that actually opened it is recorded, so the UI can show it.
    const KeyCheckSectorResult* first = session.sectorResult(0);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL(KeyCheckSectorOutcome::Recovered, first->outcome);
    TEST_ASSERT_EQUAL(MifareKeySlot::KeyB, first->slot);
    TEST_ASSERT_EQUAL(0, first->keyIndex);
}

void test_progress_advances_with_attempts_not_with_recovered_sectors() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(16));
    TEST_ASSERT_EQUAL(928u, session.report().maxAttempts);
    TEST_ASSERT_EQUAL(0, session.report().progressPercent());

    for (uint32_t attempt = 0; attempt < 464; ++attempt) {
        session.onAttempt(MifareKeySlot::KeyA,
                          static_cast<uint16_t>(attempt % MifareKeyTable::count()),
                          false);
    }
    // Half the budget spent with nothing recovered. A sector-fraction bar would
    // still read 0 % here, which is the bug this design exists to prevent.
    TEST_ASSERT_EQUAL(0, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(50, session.report().progressPercent());

    for (uint32_t attempt = 464; attempt < 928; ++attempt) {
        session.onAttempt(MifareKeySlot::KeyA, 0, false);
    }
    TEST_ASSERT_EQUAL(928u, session.report().attempts);
    TEST_ASSERT_EQUAL(100, session.report().progressPercent());
}

void test_two_keys_on_one_sector_still_count_as_one_sector() {
    // The headline number is sectors, not key hits. Opening sector 0 and then
    // also finding a Key B must not turn "1 of 2" into "2 of 2".
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(2));

    session.onAttempt(MifareKeySlot::KeyA, 0, true);
    TEST_ASSERT_EQUAL(1, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(1, session.currentSector());

    session.onAttempt(MifareKeySlot::KeyB, 0, true);
    TEST_ASSERT_EQUAL(2, session.report().sectorsRecovered);
    TEST_ASSERT_TRUE(session.report().allRecovered());
    TEST_ASSERT_EQUAL_STRING("2 of 2 sectors recovered", session.report().summary().c_str());
}

void test_sector_result_is_null_outside_the_card() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(5));
    TEST_ASSERT_NOT_NULL(session.sectorResult(0));
    TEST_ASSERT_NOT_NULL(session.sectorResult(4));
    TEST_ASSERT_NULL(session.sectorResult(5));
}

// ---------------------------------------------------------------------------
// Terminal states: "finished" is not "opened"
// ---------------------------------------------------------------------------

void test_exhausted_sectors_are_reported_as_needing_a_nested_attack() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(3));

    exhaustCurrentSector(session, MifareKeySlot::KeyA);
    TEST_ASSERT_EQUAL(1, session.report().sectorsExhausted);
    TEST_ASSERT_EQUAL(1, session.currentSector());

    exhaustCurrentSector(session, MifareKeySlot::KeyB);
    TEST_ASSERT_EQUAL(2, session.report().sectorsExhausted);
    TEST_ASSERT_EQUAL(2, session.currentSector());

    // Sector 2 opens; it was the last untried one, so the run finishes.
    session.onAttempt(MifareKeySlot::KeyA, 0, true);
    TEST_ASSERT_EQUAL(1, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(0xFF, session.currentSector());
    TEST_ASSERT_EQUAL(KeyCheckState::Finished, session.report().state);
    TEST_ASSERT_FALSE(session.report().allRecovered());
    TEST_ASSERT_EQUAL_STRING("1 of 3 sectors recovered, 2 need a nested/hardnested attack",
                             session.report().summary().c_str());
}

void test_finishing_without_opening_anything_says_so_explicitly() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(2));
    exhaustCurrentSector(session, MifareKeySlot::KeyA);
    exhaustCurrentSector(session, MifareKeySlot::KeyB);

    TEST_ASSERT_EQUAL(KeyCheckState::Finished, session.report().state);
    TEST_ASSERT_FALSE(session.report().allRecovered());
    TEST_ASSERT_EQUAL_STRING("finished with unrecovered sectors", session.report().detail);
    TEST_ASSERT_EQUAL_STRING("0 of 2 sectors recovered, 2 need a nested/hardnested attack",
                             session.report().summary().c_str());
}

void test_a_fully_opened_card_says_so_without_a_caveat() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(2));
    session.onAttempt(MifareKeySlot::KeyA, 0, true);
    session.onAttempt(MifareKeySlot::KeyB, 0, true);

    TEST_ASSERT_EQUAL(KeyCheckState::Finished, session.report().state);
    TEST_ASSERT_EQUAL(0, session.report().sectorsExhausted);
    TEST_ASSERT_TRUE(session.report().allRecovered());
    TEST_ASSERT_EQUAL_STRING("every sector recovered", session.report().detail);
    TEST_ASSERT_EQUAL_STRING("2 of 2 sectors recovered", session.report().summary().c_str());
}

void test_cancel_freezes_the_report_instead_of_claiming_anything() {
    KeyCheckSession session;
    TEST_ASSERT_TRUE(session.begin(16));
    session.onAttempt(MifareKeySlot::KeyA, 0, false);
    session.cancel();

    TEST_ASSERT_EQUAL(KeyCheckState::Cancelled, session.report().state);
    TEST_ASSERT_EQUAL_STRING("cancelled by operator", session.report().detail);

    // Nothing after a cancel may move the numbers: the operator stopped, so the
    // scan is frozen even if the driver keeps feeding it results.
    session.onAttempt(MifareKeySlot::KeyA, 5, true);
    session.onAttempt(MifareKeySlot::KeyB, 5, true);
    session.onSectorExhausted();
    TEST_ASSERT_EQUAL(1u, session.report().attempts);
    TEST_ASSERT_EQUAL(0, session.report().sectorsRecovered);
    TEST_ASSERT_EQUAL(0, session.report().sectorsExhausted);
}

void test_idle_report_is_explicit_and_division_is_safe() {
    KeyCheckReport report;
    TEST_ASSERT_EQUAL(KeyCheckState::Idle, report.state);
    // A zero denominator must not divide by zero, and must not read as "done".
    TEST_ASSERT_EQUAL(0u, report.maxAttempts);
    TEST_ASSERT_EQUAL(0, report.progressPercent());
    TEST_ASSERT_FALSE(report.allRecovered());
    TEST_ASSERT_EQUAL_STRING("key check not started", report.summary().c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_max_attempts_covers_both_slots_against_the_whole_table);
    RUN_TEST(test_preferred_key_is_the_nfc_forum_key_on_ndef_cards);
    RUN_TEST(test_begin_refuses_a_card_size_it_cannot_represent);
    RUN_TEST(test_a_sector_is_recovered_only_when_authentication_succeeds);
    RUN_TEST(test_progress_advances_with_attempts_not_with_recovered_sectors);
    RUN_TEST(test_two_keys_on_one_sector_still_count_as_one_sector);
    RUN_TEST(test_sector_result_is_null_outside_the_card);
    RUN_TEST(test_exhausted_sectors_are_reported_as_needing_a_nested_attack);
    RUN_TEST(test_finishing_without_opening_anything_says_so_explicitly);
    RUN_TEST(test_a_fully_opened_card_says_so_without_a_caveat);
    RUN_TEST(test_cancel_freezes_the_report_instead_of_claiming_anything);
    RUN_TEST(test_idle_report_is_explicit_and_division_is_safe);
    return UNITY_END();
}
