// Chameleon Ultra control model.
//
// Upstream v3.9.x added three behaviours that are worth pinning because each
// one is a place where a plausible-looking lie can enter:
//
//   * short reads are flagged, not silently passed off as a whole dump
//   * mfkey32 progress reads "N of 16" and only says complete at 16
//   * auto-connect adopts the first unit and does not let a later find steal it

#include <unity.h>

#include <string>

#include "features/rfid/ChameleonUltraModel.h"

// ---------------------------------------------------------------------------
// Short reads
// ---------------------------------------------------------------------------

void test_a_short_read_is_flagged_rather_than_passed_off_as_whole() {
    const ChameleonReadResult complete = ChameleonUltraModel::evaluateRead(16, 16);
    TEST_ASSERT_TRUE(complete.valid);
    TEST_ASSERT_FALSE(complete.partial);
    TEST_ASSERT_EQUAL(16, complete.pagesRead);
    TEST_ASSERT_EQUAL(16, complete.pagesTotal);
    TEST_ASSERT_EQUAL_STRING("complete read", complete.detail);

    const ChameleonReadResult partial = ChameleonUltraModel::evaluateRead(4, 16);
    TEST_ASSERT_TRUE(partial.valid);
    TEST_ASSERT_TRUE(partial.partial);
    TEST_ASSERT_EQUAL_STRING("partial read", partial.detail);

    // One page short is still short. An off-by-one here is how a truncated dump
    // ends up looking like a good one.
    const ChameleonReadResult nearly = ChameleonUltraModel::evaluateRead(15, 16);
    TEST_ASSERT_TRUE(nearly.valid);
    TEST_ASSERT_TRUE(nearly.partial);
}

void test_reads_that_cannot_be_true_are_not_reported_as_complete() {
    const ChameleonReadResult silent = ChameleonUltraModel::evaluateRead(0, 16);
    TEST_ASSERT_FALSE(silent.valid);
    TEST_ASSERT_EQUAL_STRING("tag did not answer", silent.detail);

    const ChameleonReadResult nothing = ChameleonUltraModel::evaluateRead(0, 0);
    TEST_ASSERT_FALSE(nothing.valid);
    TEST_ASSERT_EQUAL_STRING("no pages expected", nothing.detail);

    // More pages than the card has is a driver miscount. Reporting "complete
    // read" would put a confident label on an impossible number.
    const ChameleonReadResult impossible = ChameleonUltraModel::evaluateRead(20, 16);
    TEST_ASSERT_FALSE(impossible.valid);
    TEST_ASSERT_FALSE(impossible.partial);
    TEST_ASSERT_EQUAL_STRING("page count inconsistent", impossible.detail);
}

// ---------------------------------------------------------------------------
// mfkey32 progress
// ---------------------------------------------------------------------------

void test_mfkey32_progress_counts_to_sixteen_and_then_stops() {
    Mfkey32Progress progress = ChameleonUltraModel::resetProgress();
    TEST_ASSERT_EQUAL(0, progress.nonces);
    TEST_ASSERT_FALSE(progress.complete);
    TEST_ASSERT_EQUAL(16, progress.remaining());

    // 15 of 16 must NOT read as done -- "enough to try" is not "finished".
    for (uint8_t index = 0; index < 15; ++index) {
        progress = ChameleonUltraModel::recordNonce(progress);
    }
    TEST_ASSERT_EQUAL(15, progress.nonces);
    TEST_ASSERT_FALSE(progress.complete);
    TEST_ASSERT_EQUAL(1, progress.remaining());

    progress = ChameleonUltraModel::recordNonce(progress);
    TEST_ASSERT_EQUAL(16, progress.nonces);
    TEST_ASSERT_TRUE(progress.complete);
    TEST_ASSERT_EQUAL(0, progress.remaining());

    // Further nonces saturate instead of wrapping the 8-bit counter to 0, which
    // would reset the reported progress mid-capture.
    progress = ChameleonUltraModel::recordNonce(progress);
    TEST_ASSERT_EQUAL(16, progress.nonces);
    TEST_ASSERT_TRUE(progress.complete);
    TEST_ASSERT_EQUAL(0, progress.remaining());
}

// ---------------------------------------------------------------------------
// Auto-connect
// ---------------------------------------------------------------------------

void test_begin_starts_scanning_with_every_slot_empty() {
    ChameleonSession session;
    session.begin();
    TEST_ASSERT_EQUAL(ChameleonState::Scanning, session.state());
    TEST_ASSERT_EQUAL_STRING("", session.address().c_str());
    TEST_ASSERT_EQUAL(0, session.loadedSlotCount());
    TEST_ASSERT_EQUAL(0, session.firstFreeSlot());
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        TEST_ASSERT_FALSE(session.slot(index).loaded);
        TEST_ASSERT_EQUAL(ChameleonSlotKind::Empty, session.slot(index).kind);
        TEST_ASSERT_EQUAL(index, session.slot(index).index);
    }
}

void test_the_first_unit_found_is_adopted_and_a_later_one_is_ignored() {
    ChameleonSession session;
    session.begin();

    TEST_ASSERT_TRUE(session.onDeviceFound("AA:BB:CC:DD:EE:01"));
    TEST_ASSERT_EQUAL(ChameleonState::Ready, session.state());
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:01", session.address().c_str());

    // v3.9.2 auto-connect: no pick list, and the second find must not take over
    // a connection that is already held.
    TEST_ASSERT_FALSE(session.onDeviceFound("AA:BB:CC:DD:EE:02"));
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:01", session.address().c_str());
    TEST_ASSERT_EQUAL(ChameleonState::Ready, session.state());
}

void test_a_find_with_no_address_is_not_adopted() {
    ChameleonSession session;
    session.begin();
    // Adopting this would leave the session Ready with nothing to talk to.
    TEST_ASSERT_FALSE(session.onDeviceFound(""));
    TEST_ASSERT_EQUAL(ChameleonState::Scanning, session.state());
    TEST_ASSERT_EQUAL_STRING("", session.address().c_str());
}

void test_stop_releases_the_unit_and_a_connect_failure_is_reported() {
    ChameleonSession session;
    session.begin();
    TEST_ASSERT_TRUE(session.onDeviceFound("AA:BB:CC:DD:EE:01"));
    session.stop();
    TEST_ASSERT_EQUAL(ChameleonState::Idle, session.state());
    TEST_ASSERT_EQUAL_STRING("", session.address().c_str());

    // Once released, a new unit can be adopted.
    session.begin();
    TEST_ASSERT_TRUE(session.onDeviceFound("AA:BB:CC:DD:EE:03"));

    session.onConnectFailed("gatt 133");
    TEST_ASSERT_EQUAL(ChameleonState::Failed, session.state());
    TEST_ASSERT_EQUAL_STRING("gatt 133", session.lastError());

    session.onConnectFailed(nullptr);
    TEST_ASSERT_EQUAL_STRING("chameleon connect failed", session.lastError());
}

// ---------------------------------------------------------------------------
// The eight-slot table
// ---------------------------------------------------------------------------

void test_slots_are_assigned_cleared_and_never_wrapped() {
    ChameleonSession session;
    session.begin();

    TEST_ASSERT_TRUE(session.setSlot(3, ChameleonSlotKind::MifareClassic1K, "office"));
    TEST_ASSERT_TRUE(session.slot(3).loaded);
    TEST_ASSERT_EQUAL(ChameleonSlotKind::MifareClassic1K, session.slot(3).kind);
    TEST_ASSERT_EQUAL_STRING("office", session.slot(3).nickname.c_str());
    TEST_ASSERT_EQUAL(1, session.loadedSlotCount());
    TEST_ASSERT_EQUAL(0, session.firstFreeSlot());

    // An out-of-range index must be refused, not wrapped onto slot 0.
    TEST_ASSERT_FALSE(session.setSlot(kChameleonSlotCount, ChameleonSlotKind::Ntag21x, "bad"));
    TEST_ASSERT_EQUAL_STRING("slot index out of range", session.lastError());
    TEST_ASSERT_EQUAL(1, session.loadedSlotCount());
    TEST_ASSERT_FALSE(session.slot(0).loaded);

    TEST_ASSERT_FALSE(session.clearSlot(kChameleonSlotCount));
    TEST_ASSERT_EQUAL_STRING("slot index out of range", session.lastError());

    // Out-of-range reads hand back a blank slot rather than a neighbour's data.
    TEST_ASSERT_FALSE(session.slot(200).loaded);
    TEST_ASSERT_EQUAL(ChameleonSlotKind::Empty, session.slot(200).kind);

    TEST_ASSERT_TRUE(session.clearSlot(3));
    TEST_ASSERT_FALSE(session.slot(3).loaded);
    TEST_ASSERT_EQUAL(0, session.loadedSlotCount());
    TEST_ASSERT_EQUAL(0, session.firstFreeSlot());
}

void test_all_eight_slots_can_be_loaded_and_the_ninth_has_nowhere_to_go() {
    ChameleonSession session;
    session.begin();
    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        TEST_ASSERT_EQUAL(index, session.firstFreeSlot());
        TEST_ASSERT_TRUE(session.setSlot(index, ChameleonSlotKind::MifareUltralight, "tag"));
    }

    TEST_ASSERT_EQUAL(8, session.loadedSlotCount());
    // 0xFF is the documented "no free slot", not 0 and not 8.
    TEST_ASSERT_EQUAL(0xFF, session.firstFreeSlot());

    // Writing the kind Empty must clear the slot even when a name is supplied.
    TEST_ASSERT_TRUE(session.setSlot(5, ChameleonSlotKind::Empty, "stale name"));
    TEST_ASSERT_FALSE(session.slot(5).loaded);
    TEST_ASSERT_EQUAL(7, session.loadedSlotCount());
    TEST_ASSERT_EQUAL(5, session.firstFreeSlot());
}

// ---------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------

void test_every_slot_kind_and_state_has_a_label() {
    TEST_ASSERT_EQUAL_STRING("Empty", ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::Empty));
    TEST_ASSERT_EQUAL_STRING("MIFARE Classic 1K",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::MifareClassic1K));
    TEST_ASSERT_EQUAL_STRING("MIFARE Classic 4K",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::MifareClassic4K));
    TEST_ASSERT_EQUAL_STRING("NTAG 21x",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::Ntag21x));
    TEST_ASSERT_EQUAL_STRING("ISO 15693",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::Iso15693));
    TEST_ASSERT_EQUAL_STRING("EM4100 (LF)",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::Em4100));
    TEST_ASSERT_EQUAL_STRING("HID Prox (LF)",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::HidProx));
    TEST_ASSERT_EQUAL_STRING("Unrecognised",
                             ChameleonUltraModel::slotKindLabel(ChameleonSlotKind::Unknown));

    TEST_ASSERT_EQUAL_STRING("IDLE", ChameleonUltraModel::stateLabel(ChameleonState::Idle));
    TEST_ASSERT_EQUAL_STRING("SCANNING", ChameleonUltraModel::stateLabel(ChameleonState::Scanning));
    TEST_ASSERT_EQUAL_STRING("CONNECTING",
                             ChameleonUltraModel::stateLabel(ChameleonState::Connecting));
    TEST_ASSERT_EQUAL_STRING("READY", ChameleonUltraModel::stateLabel(ChameleonState::Ready));
    TEST_ASSERT_EQUAL_STRING("BUSY", ChameleonUltraModel::stateLabel(ChameleonState::Busy));
    TEST_ASSERT_EQUAL_STRING("FAILED", ChameleonUltraModel::stateLabel(ChameleonState::Failed));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_a_short_read_is_flagged_rather_than_passed_off_as_whole);
    RUN_TEST(test_reads_that_cannot_be_true_are_not_reported_as_complete);
    RUN_TEST(test_mfkey32_progress_counts_to_sixteen_and_then_stops);
    RUN_TEST(test_begin_starts_scanning_with_every_slot_empty);
    RUN_TEST(test_the_first_unit_found_is_adopted_and_a_later_one_is_ignored);
    RUN_TEST(test_a_find_with_no_address_is_not_adopted);
    RUN_TEST(test_stop_releases_the_unit_and_a_connect_failure_is_reported);
    RUN_TEST(test_slots_are_assigned_cleared_and_never_wrapped);
    RUN_TEST(test_all_eight_slots_can_be_loaded_and_the_ninth_has_nowhere_to_go);
    RUN_TEST(test_every_slot_kind_and_state_has_a_label);
    return UNITY_END();
}
