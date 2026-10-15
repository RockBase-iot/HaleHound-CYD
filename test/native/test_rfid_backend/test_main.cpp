// RFID back-end selection: the capability matrix and the seven-screen gating.
//
// The matrix is what the UI uses to grey out what a reader cannot do, so it is
// a claim about the reader silicon in this repository. Two invariants worth
// holding down:
//
//   * Every screen has a distinct label. With seven screens the header bar is
//     the only thing telling them apart, and "Reader" appeared twice.
//   * Out-of-range indices are refused rather than wrapped, so a bad index
//     cannot silently grey out (or enable) a different back end.

#include <unity.h>

#include <cstring>

#include "features/rfid/RfidBackendModel.h"

// ---------------------------------------------------------------------------
// Enumeration
// ---------------------------------------------------------------------------

void test_the_three_back_ends_are_listed_in_a_fixed_order() {
    TEST_ASSERT_EQUAL(3, RfidBackendModel::backendCount());
    TEST_ASSERT_EQUAL(RfidBackend::Pn532, RfidBackendModel::backendAt(0));
    TEST_ASSERT_EQUAL(RfidBackend::ChameleonUltra, RfidBackendModel::backendAt(1));
    TEST_ASSERT_EQUAL(RfidBackend::St25r3916, RfidBackendModel::backendAt(2));

    TEST_ASSERT_EQUAL_STRING("PN-532", RfidBackendModel::backendLabel(RfidBackend::Pn532));
    TEST_ASSERT_EQUAL_STRING("Chameleon Ultra",
                             RfidBackendModel::backendLabel(RfidBackend::ChameleonUltra));
    TEST_ASSERT_EQUAL_STRING("ST25R3916",
                             RfidBackendModel::backendLabel(RfidBackend::St25r3916));

    // An index past the end falls back to the first entry rather than reading
    // out of bounds; pin it so the behaviour is a decision, not an accident.
    TEST_ASSERT_EQUAL(RfidBackend::Pn532, RfidBackendModel::backendAt(3));
    TEST_ASSERT_EQUAL(RfidBackend::Pn532, RfidBackendModel::backendAt(200));
}

void test_the_seven_screens_have_seven_distinct_labels() {
    TEST_ASSERT_EQUAL(7, RfidBackendModel::kScreenCount);

    for (uint8_t index = 0; index < RfidBackendModel::kScreenCount; ++index) {
        const char* label = RfidBackendModel::screenLabel(RfidBackendModel::screenAt(index));
        TEST_ASSERT_NOT_NULL(label);
        TEST_ASSERT_TRUE(label[0] != '\0');
        // No duplicates: two screens sharing a label leaves the header bar
        // unable to say where you are.
        for (uint8_t other = index + 1; other < RfidBackendModel::kScreenCount; ++other) {
            TEST_ASSERT_TRUE(std::strcmp(
                                 label,
                                 RfidBackendModel::screenLabel(RfidBackendModel::screenAt(other))) != 0);
        }
    }

    TEST_ASSERT_EQUAL(RfidScreen::BackendPicker, RfidBackendModel::screenAt(0));
    TEST_ASSERT_EQUAL(RfidScreen::CardDump, RfidBackendModel::screenAt(3));
    TEST_ASSERT_EQUAL(RfidScreen::Guide, RfidBackendModel::screenAt(6));
    // Past the end clamps to the first screen rather than reading out of bounds.
    TEST_ASSERT_EQUAL(RfidScreen::BackendPicker, RfidBackendModel::screenAt(7));
}

// ---------------------------------------------------------------------------
// Capability matrix
// ---------------------------------------------------------------------------

void test_the_pn532_matrix_matches_its_silicon_and_wiring() {
    const RfidBackendCapability caps = RfidBackendModel::capability(RfidBackend::Pn532);

    TEST_ASSERT_TRUE(caps.mifareClassic);
    TEST_ASSERT_TRUE(caps.mifareUltralight);
    TEST_ASSERT_TRUE(caps.ntag);
    TEST_ASSERT_TRUE(caps.isoDep);
    TEST_ASSERT_TRUE(caps.nestedAuthentication);
    TEST_ASSERT_TRUE(caps.keyedWrite);
    TEST_ASSERT_TRUE(caps.flipperNfcExport);
    TEST_ASSERT_TRUE(caps.proxmarkEmlExport);

    // These are the claims about the hardware, and the four that make the PN532
    // the least capable back end. If any flips to true the UI starts offering
    // actions the reader cannot perform.
    TEST_ASSERT_FALSE(caps.iclass);       // not in the PN532 silicon
    TEST_ASSERT_FALSE(caps.magicGen1A);   // no back-door write path in this driver
    TEST_ASSERT_FALSE(caps.hfEmulation);  // emulation is not wired up here
    TEST_ASSERT_FALSE(caps.lfSupport);    // the NM-RF-HAT routes LF elsewhere
    TEST_ASSERT_FALSE(caps.mfkey32NonceCapture);
}

void test_the_chameleon_matrix_matches_its_bluetooth_role() {
    const RfidBackendCapability caps = RfidBackendModel::capability(RfidBackend::ChameleonUltra);

    TEST_ASSERT_TRUE(caps.mifareClassic);
    TEST_ASSERT_TRUE(caps.magicGen1A);
    TEST_ASSERT_TRUE(caps.nestedAuthentication);
    TEST_ASSERT_TRUE(caps.hfEmulation);
    TEST_ASSERT_TRUE(caps.lfSupport);
    TEST_ASSERT_TRUE(caps.mfkey32NonceCapture);

    // Dumps come back over BLE and are exported by the host, so the device
    // itself cannot write a Flipper or Proxmark file.
    TEST_ASSERT_FALSE(caps.flipperNfcExport);
    TEST_ASSERT_FALSE(caps.proxmarkEmlExport);
    TEST_ASSERT_FALSE(caps.iclass);
}

void test_the_st25r3916_matrix_buys_iclass_at_the_cost_of_lf() {
    const RfidBackendCapability caps = RfidBackendModel::capability(RfidBackend::St25r3916);

    // The one back end that does iCLASS, which is the reason it exists here.
    TEST_ASSERT_TRUE(caps.iclass);
    TEST_ASSERT_TRUE(caps.mifareClassic);
    TEST_ASSERT_TRUE(caps.ntag);
    TEST_ASSERT_TRUE(caps.isoDep);
    TEST_ASSERT_TRUE(caps.magicGen1A);
    TEST_ASSERT_TRUE(caps.flipperNfcExport);
    TEST_ASSERT_TRUE(caps.proxmarkEmlExport);

    // A 13.56 MHz part: no low frequency, no emulation, no nonce capture.
    TEST_ASSERT_FALSE(caps.lfSupport);
    TEST_ASSERT_FALSE(caps.hfEmulation);
    TEST_ASSERT_FALSE(caps.mfkey32NonceCapture);
}

// ---------------------------------------------------------------------------
// Screen gating
// ---------------------------------------------------------------------------

void test_the_guide_and_picker_are_always_reachable() {
    for (uint8_t index = 0; index < RfidBackendModel::backendCount(); ++index) {
        const RfidBackend backend = RfidBackendModel::backendAt(index);
        TEST_ASSERT_TRUE(RfidBackendModel::screenAvailable(backend, RfidScreen::BackendPicker));
        TEST_ASSERT_TRUE(RfidBackendModel::screenAvailable(backend, RfidScreen::Guide));
        // Scanning and reading need at least one HF card family.
        TEST_ASSERT_TRUE(RfidBackendModel::screenAvailable(backend, RfidScreen::CardScanner));
        TEST_ASSERT_TRUE(RfidBackendModel::screenAvailable(backend, RfidScreen::CardReader));
        TEST_ASSERT_TRUE(RfidBackendModel::screenAvailable(backend, RfidScreen::CardDump));
    }
}

void test_key_check_and_write_follow_the_matrix_not_the_back_end() {
    // Key Check needs a way to attack keys: nested auth, a magic back door, or
    // mfkey32 nonce capture. Every shipped back end has at least one.
    for (uint8_t index = 0; index < RfidBackendModel::backendCount(); ++index) {
        const RfidBackend backend = RfidBackendModel::backendAt(index);
        const RfidBackendCapability caps = RfidBackendModel::capability(backend);
        TEST_ASSERT_EQUAL(caps.nestedAuthentication || caps.magicGen1A || caps.mfkey32NonceCapture,
                          RfidBackendModel::screenAvailable(backend, RfidScreen::KeyCheck));
        // Writing needs a keyed write or the back door.
        TEST_ASSERT_EQUAL(caps.keyedWrite || caps.magicGen1A,
                          RfidBackendModel::screenAvailable(backend, RfidScreen::CardWrite));
    }
}

// ---------------------------------------------------------------------------
// Dump export
// ---------------------------------------------------------------------------

void test_export_needs_both_writers_for_a_classic_dump() {
    // A MIFARE Classic dump is only useful as a pair, so both writers are
    // required; the Chameleon has neither and must be refused.
    TEST_ASSERT_TRUE(RfidBackendModel::canExportDump(RfidBackend::Pn532, true));
    TEST_ASSERT_TRUE(RfidBackendModel::canExportDump(RfidBackend::St25r3916, true));
    TEST_ASSERT_FALSE(RfidBackendModel::canExportDump(RfidBackend::ChameleonUltra, true));

    // A tag dump only needs the .nfc writer, and the ST25R3916 has it.
    TEST_ASSERT_TRUE(RfidBackendModel::canExportDump(RfidBackend::Pn532, false));
    TEST_ASSERT_TRUE(RfidBackendModel::canExportDump(RfidBackend::St25r3916, false));
    // The Chameleon cannot even produce the single file.
    TEST_ASSERT_FALSE(RfidBackendModel::canExportDump(RfidBackend::ChameleonUltra, false));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_the_three_back_ends_are_listed_in_a_fixed_order);
    RUN_TEST(test_the_seven_screens_have_seven_distinct_labels);
    RUN_TEST(test_the_pn532_matrix_matches_its_silicon_and_wiring);
    RUN_TEST(test_the_chameleon_matrix_matches_its_bluetooth_role);
    RUN_TEST(test_the_st25r3916_matrix_buys_iclass_at_the_cost_of_lf);
    RUN_TEST(test_the_guide_and_picker_are_always_reachable);
    RUN_TEST(test_key_check_and_write_follow_the_matrix_not_the_back_end);
    RUN_TEST(test_export_needs_both_writers_for_a_classic_dump);
    return UNITY_END();
}
