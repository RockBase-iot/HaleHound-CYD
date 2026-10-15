#include <unity.h>

#include <string>

#include "features/rfid/IclassPacs.h"

// 26-bit H10301 credential, built by hand so the expected facility/card values
// and both parity bits are independently checkable:
//   P0(0) F(10101011) C(0001001000110100) P1(1)
// P0 is an even-parity bit over the 12 bits that follow it; those carry six
// ones, so P0 is 0. P1 is an odd-parity bit over the 12 bits before it; those
// carry four ones, so P1 must be 1 to make the total odd.
static const char* const kHid26Bits = "0 1010 1011 0001 0010 0011 0100 1";

void test_hid26_fields_and_parity() {
    const PacsDecoded decoded = IclassPacs::decodeBitString(kHid26Bits);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(PacsFormatId::Hid26, decoded.format);
    TEST_ASSERT_EQUAL(26, decoded.bitString.size() > 0 ? 26 : 0);
    TEST_ASSERT_EQUAL(0xAB, decoded.facilityCode);
    TEST_ASSERT_EQUAL(0x1234, static_cast<unsigned>(decoded.cardNumber));
    TEST_ASSERT_TRUE(decoded.parityChecked);
    TEST_ASSERT_TRUE(decoded.leadingParityOk);
    TEST_ASSERT_TRUE(decoded.trailingParityOk);
    TEST_ASSERT_EQUAL_STRING("171:4660", IclassPacs::formatCredential(decoded).c_str());
    TEST_ASSERT_EQUAL_STRING("HID 26-bit (H10301)", IclassPacs::formatLabel(decoded.format));
}

void test_hid26_flipped_parity_is_reported() {
    // Same credential with the leading parity bit inverted.
    const PacsDecoded decoded = IclassPacs::decodeBitString("1 1010 1011 0001 0010 0011 0100 1");
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_TRUE(decoded.parityChecked);
    TEST_ASSERT_FALSE(decoded.leadingParityOk);
    // The fields still decode; only the parity verdict changes.
    TEST_ASSERT_EQUAL(0xAB, decoded.facilityCode);
}

void test_parity_helpers_use_the_group_count_not_the_bit() {
    // Eight ones in bits 0..7: the group itself is even.
    TEST_ASSERT_TRUE(IclassPacs::evenParity(0xFF, 0, 7));
    TEST_ASSERT_FALSE(IclassPacs::oddParity(0xFF, 0, 7));
    // Seven ones is odd.
    TEST_ASSERT_FALSE(IclassPacs::evenParity(0x7F, 0, 7));
    TEST_ASSERT_TRUE(IclassPacs::oddParity(0x7F, 0, 7));
    // A reversed or out-of-range window is refused rather than silently zero.
    TEST_ASSERT_FALSE(IclassPacs::evenParity(0xFF, 7, 0));
}

void test_wider_formats_decode_but_do_not_claim_confirmed_parity() {
    // 35-bit Corporate 1000: 12-bit corporate code (0x0ABC) and 20-bit card.
    const uint64_t corporate = 0x0ABCULL << 22;
    const uint64_t card35 = 0x12345ULL << 2;
    const PacsDecoded decoded = IclassPacs::decode(corporate | card35, 35);
    TEST_ASSERT_TRUE(decoded.valid);
    TEST_ASSERT_EQUAL(PacsFormatId::Corporate1000, decoded.format);
    TEST_ASSERT_EQUAL(0x0ABC, decoded.facilityCode);
    TEST_ASSERT_EQUAL(0x12345, static_cast<unsigned>(decoded.cardNumber));
    // The parity convention for this layout is not confirmed, so it must not be
    // reported as checked.
    TEST_ASSERT_FALSE(decoded.parityChecked);
    TEST_ASSERT_EQUAL_STRING("Corporate 1000 (35-bit)", IclassPacs::formatLabel(decoded.format));

    const uint64_t site48 = 0x2AAA55ULL << 25;
    const uint64_t card48 = 0xABCDEFULL << 1;
    const PacsDecoded wide = IclassPacs::decode(site48 | card48, 48);
    TEST_ASSERT_TRUE(wide.valid);
    TEST_ASSERT_EQUAL(PacsFormatId::Hid48, wide.format);
    TEST_ASSERT_EQUAL(0x2AAA55, wide.facilityCode);
    TEST_ASSERT_EQUAL(0xABCDEF, static_cast<unsigned>(wide.cardNumber));
    TEST_ASSERT_FALSE(wide.parityChecked);
}

void test_unknown_width_is_unsupported_framing_not_a_silent_card() {
    const PacsDecoded decoded = IclassPacs::decode(0x1234, 13);
    TEST_ASSERT_FALSE(decoded.valid);
    TEST_ASSERT_EQUAL(PacsFormatId::Unknown, decoded.format);
    TEST_ASSERT_EQUAL_STRING("unsupported framing", decoded.detail);
}

void test_bit_string_parsing_rejects_garbage_and_whitespace_is_ignored() {
    uint64_t raw = 0;
    uint8_t bits = 0;
    TEST_ASSERT_TRUE(IclassPacs::parseBitString("1010_1010 1010", raw, bits));
    TEST_ASSERT_EQUAL(12, bits);
    TEST_ASSERT_EQUAL(0xAAA, static_cast<unsigned>(raw));

    TEST_ASSERT_FALSE(IclassPacs::parseBitString("1012", raw, bits));
    TEST_ASSERT_FALSE(IclassPacs::parseBitString("", raw, bits));

    TEST_ASSERT_EQUAL_STRING("1010 1010", IclassPacs::toBitString(0xAA, 8).c_str());
    TEST_ASSERT_EQUAL_STRING("", IclassPacs::toBitString(0, 0).c_str());
}

void test_lookup_by_bit_count() {
    TEST_ASSERT_NOT_NULL(IclassPacs::formatForBitCount(26));
    TEST_ASSERT_NOT_NULL(IclassPacs::formatForBitCount(35));
    TEST_ASSERT_NOT_NULL(IclassPacs::formatForBitCount(48));
    TEST_ASSERT_NULL(IclassPacs::formatForBitCount(64));
    TEST_ASSERT_EQUAL(PacsFormatId::Hid26, IclassPacs::formatForBitCount(26)->id);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_hid26_fields_and_parity);
    RUN_TEST(test_hid26_flipped_parity_is_reported);
    RUN_TEST(test_parity_helpers_use_the_group_count_not_the_bit);
    RUN_TEST(test_wider_formats_decode_but_do_not_claim_confirmed_parity);
    RUN_TEST(test_unknown_width_is_unsupported_framing_not_a_silent_card);
    RUN_TEST(test_bit_string_parsing_rejects_garbage_and_whitespace_is_ignored);
    RUN_TEST(test_lookup_by_bit_count);
    return UNITY_END();
}
