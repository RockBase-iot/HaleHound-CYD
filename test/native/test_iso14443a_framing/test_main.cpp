// ISO14443-A framing and command bytes.
//
// Everything here is a byte sequence a card or a reader puts on the wire, so
// every assertion is a pinned constant rather than a property. Two of them are
// worth calling out because they are the ones a plausible implementation gets
// wrong:
//
//   * CRC_A. The catalogue parameterisation is init 0xC6C6, and a reflected
//     register holds that byte-swapped as 0x6363. Both forms give check value
//     0xBF05 over "123456789", and this suite pins that value so a future
//     "cleanup" of the constant fails loudly.
//   * The ATQA UID-size field. It lives in bits b8/b7, which are bits 7 and 6
//     of the FIRST ATQA byte -- so MIFARE Ultralight's 00 44 means a 7-byte
//     UID even though the high byte is zero. Reading the 16-bit value's top
//     bits instead makes every Ultralight look like a 4-byte card.

#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "features/rfid/Iso14443aFraming.h"

namespace {

// The CRC-16/ISO-IEC-14443-3-A definition written the way the catalogue writes
// it, LSB first, with no lookup table and no nibble trick. Deliberately a
// second implementation: if the shipped one and this one ever disagree, one of
// them is wrong and the suite says so.
uint16_t referenceCrcA(const std::vector<uint8_t>& data) {
    uint16_t crc = 0x6363;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001U) != 0U) {
                crc = static_cast<uint16_t>((crc >> 1) ^ 0x8408U);
            } else {
                crc = static_cast<uint16_t>(crc >> 1);
            }
        }
    }
    return crc;
}

uint32_t nextRandom(uint32_t& state) {
    // xorshift32: deterministic, so a failure is reproducible.
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

std::string hex(const uint8_t* data, size_t length) {
    static const char* digits = "0123456789ABCDEF";
    std::string out;
    for (size_t i = 0; i < length; ++i) {
        out.push_back(digits[(data[i] >> 4) & 0x0F]);
        out.push_back(digits[data[i] & 0x0F]);
        out.push_back(' ');
    }
    if (!out.empty()) {
        out.pop_back();
    }
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// CRC_A
// ---------------------------------------------------------------------------

void test_crc_a_matches_the_catalogue_check_value() {
    const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_UINT16(0xBF05U, rfidCrcA(check, sizeof(check)));
}

void test_crc_a_agrees_with_an_independent_implementation() {
    uint32_t state = 0x12345678U;
    for (int iteration = 0; iteration < 500; ++iteration) {
        const size_t length = nextRandom(state) % 65U;
        std::vector<uint8_t> data(length);
        for (size_t i = 0; i < length; ++i) {
            data[i] = static_cast<uint8_t>(nextRandom(state) & 0xFFU);
        }
        const uint16_t expected = referenceCrcA(data);
        const uint16_t actual = rfidCrcA(data.data(), static_cast<uint16_t>(length));
        if (expected != actual) {
            TEST_FAIL_MESSAGE(("CRC mismatch on: " + hex(data.data(), data.size())).c_str());
        }
    }
}

void test_crc_a_ntag_read_frame_vector() {
    // READ page 4 is 30 04; the CRC goes out least significant byte first, so
    // the frame on the wire is 30 04 26 EE. This is the one vector in this
    // suite that comes from the wire rather than from a formula.
    const uint8_t command[] = {0x30, 0x04};
    TEST_ASSERT_EQUAL_UINT16(0xEE26U, rfidCrcA(command, sizeof(command)));

    uint8_t frame[4] = {0x30, 0x04, 0x00, 0x00};
    rfidAppendCrcA(frame, 2);
    TEST_ASSERT_EQUAL_UINT8(0x26U, frame[2]);
    TEST_ASSERT_EQUAL_UINT8(0xEEU, frame[3]);
    TEST_ASSERT_TRUE(rfidCheckCrcA(frame, sizeof(frame)));
}

void test_crc_check_rejects_a_flipped_bit_and_a_short_frame() {
    uint8_t frame[4] = {0x30, 0x04, 0x00, 0x00};
    rfidAppendCrcA(frame, 2);
    frame[1] ^= 0x01U;
    TEST_ASSERT_FALSE(rfidCheckCrcA(frame, sizeof(frame)));
    // A frame shorter than the CRC itself can never check out.
    TEST_ASSERT_FALSE(rfidCheckCrcA(frame, 1));
    TEST_ASSERT_FALSE(rfidCheckCrcA(frame, 0));
}

// ---------------------------------------------------------------------------
// ISO14443-3
// ---------------------------------------------------------------------------

void test_reqA_and_wupA_are_single_bytes() {
    uint8_t buffer[8] = {};
    TEST_ASSERT_EQUAL_UINT16(1, Iso14443aFraming::buildReqA(buffer));
    TEST_ASSERT_EQUAL_UINT8(0x26U, buffer[0]);
    TEST_ASSERT_EQUAL_UINT16(1, Iso14443aFraming::buildWupA(buffer));
    TEST_ASSERT_EQUAL_UINT8(0x52U, buffer[0]);
}

void test_cascade_geometry_follows_the_uid_length() {
    TEST_ASSERT_EQUAL_UINT8(1, Iso14443aFraming::cascadeLevelCount(4));
    TEST_ASSERT_EQUAL_UINT8(2, Iso14443aFraming::cascadeLevelCount(7));
    TEST_ASSERT_EQUAL_UINT8(3, Iso14443aFraming::cascadeLevelCount(10));
    // No ISO14443-3 card has any other UID length.
    TEST_ASSERT_EQUAL_UINT8(0, Iso14443aFraming::cascadeLevelCount(5));
    TEST_ASSERT_EQUAL_UINT8(0, Iso14443aFraming::cascadeLevelCount(0));

    TEST_ASSERT_EQUAL_UINT8(4, Iso14443aFraming::bytesPerCascadeLevel(1));
    TEST_ASSERT_EQUAL_UINT8(4, Iso14443aFraming::bytesPerCascadeLevel(2));
    TEST_ASSERT_EQUAL_UINT8(2, Iso14443aFraming::bytesPerCascadeLevel(3));
    TEST_ASSERT_EQUAL_UINT8(0, Iso14443aFraming::bytesPerCascadeLevel(4));

    TEST_ASSERT_EQUAL_UINT8(0x93U, Iso14443aFraming::cascadeCommand(1));
    TEST_ASSERT_EQUAL_UINT8(0x95U, Iso14443aFraming::cascadeCommand(2));
    TEST_ASSERT_EQUAL_UINT8(0x97U, Iso14443aFraming::cascadeCommand(3));
    TEST_ASSERT_EQUAL_UINT8(0x00U, Iso14443aFraming::cascadeCommand(0));
}

void test_block_check_character_is_the_xor_of_its_four_bytes() {
    const uint8_t bytes[4] = {0x04, 0x12, 0x34, 0x56};
    TEST_ASSERT_EQUAL_UINT8(0x04U ^ 0x12U ^ 0x34U ^ 0x56U,
                            Iso14443aFraming::blockCheckCharacter(bytes));
    TEST_ASSERT_EQUAL_UINT8(0x00U, Iso14443aFraming::blockCheckCharacter(nullptr));
}

void test_select_for_a_four_byte_uid() {
    const uint8_t uid[4] = {0x04, 0x12, 0x34, 0x56};
    uint8_t frame[16] = {};
    const uint16_t length = Iso14443aFraming::buildSelect(1, uid, 4, frame);
    // One level: cmd, NVB 0x70, four UID bytes, BCC, then the CRC -- 9 bytes.
    TEST_ASSERT_EQUAL_UINT16(9, length);
    const uint8_t expected[7] = {0x93, 0x70, 0x04, 0x12, 0x34, 0x56, 0x74};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(frame, expected, sizeof(expected)));
    TEST_ASSERT_TRUE(rfidCheckCrcA(frame, length));
}

void test_select_for_a_seven_byte_uid_uses_the_cascade_tag() {
    const uint8_t uid[7] = {0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    uint8_t frame[16] = {};

    const uint16_t first = Iso14443aFraming::buildSelect(1, uid, 7, frame);
    TEST_ASSERT_EQUAL_UINT16(9, first);
    // Level 1 carries the cascade tag instead of the fourth UID byte.
    const uint8_t expectedFirst[7] = {0x93, 0x70, 0x88, 0x04, 0x12, 0x34,
                                      0x88U ^ 0x04U ^ 0x12U ^ 0x34U};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(frame, expectedFirst, sizeof(expectedFirst)));
    TEST_ASSERT_TRUE(rfidCheckCrcA(frame, first));

    const uint16_t second = Iso14443aFraming::buildSelect(2, uid, 7, frame);
    TEST_ASSERT_EQUAL_UINT16(9, second);
    // Level 2 carries the last four UID bytes and no cascade tag.
    const uint8_t expectedSecond[7] = {0x95, 0x70, 0x56, 0x78, 0x9A, 0xBC,
                                       0x56U ^ 0x78U ^ 0x9AU ^ 0xBCU};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(frame, expectedSecond, sizeof(expectedSecond)));
    TEST_ASSERT_TRUE(rfidCheckCrcA(frame, second));
}

void test_select_for_a_ten_byte_uid_uses_three_levels() {
    const uint8_t uid[10] = {0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    uint8_t frame[16] = {};

    TEST_ASSERT_EQUAL_UINT16(9, Iso14443aFraming::buildSelect(1, uid, 10, frame));
    TEST_ASSERT_EQUAL_UINT8(0x93U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x88U, frame[2]);  // cascade tag
    TEST_ASSERT_EQUAL_UINT8(0x04U, frame[3]);  // uid[0]
    TEST_ASSERT_EQUAL_UINT8(0x11U, frame[4]);  // uid[1]
    TEST_ASSERT_EQUAL_UINT8(0x22U, frame[5]);  // uid[2]

    TEST_ASSERT_EQUAL_UINT16(9, Iso14443aFraming::buildSelect(2, uid, 10, frame));
    TEST_ASSERT_EQUAL_UINT8(0x95U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x88U, frame[2]);  // cascade tag again
    TEST_ASSERT_EQUAL_UINT8(0x33U, frame[3]);  // uid[3]
    TEST_ASSERT_EQUAL_UINT8(0x44U, frame[4]);  // uid[4]
    TEST_ASSERT_EQUAL_UINT8(0x55U, frame[5]);  // uid[5]

    TEST_ASSERT_EQUAL_UINT16(9, Iso14443aFraming::buildSelect(3, uid, 10, frame));
    TEST_ASSERT_EQUAL_UINT8(0x97U, frame[0]);
    // The final level has no cascade tag: uid[6..9], four real bytes.
    TEST_ASSERT_EQUAL_UINT8(0x66U, frame[2]);
    TEST_ASSERT_EQUAL_UINT8(0x77U, frame[3]);
    TEST_ASSERT_EQUAL_UINT8(0x88U, frame[4]);
    TEST_ASSERT_EQUAL_UINT8(0x99U, frame[5]);
    TEST_ASSERT_TRUE(rfidCheckCrcA(frame, 9));
}

void test_select_rejects_a_level_past_the_uid() {
    const uint8_t uid[4] = {0x04, 0x12, 0x34, 0x56};
    uint8_t frame[16] = {};
    // A 4-byte UID has one level, so level 2 has nothing to select.
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildSelect(2, uid, 4, frame));
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildSelect(0, uid, 4, frame));
    // A UID length no card uses cannot be framed.
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildSelect(1, uid, 5, frame));
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildSelect(1, nullptr, 4, frame));
}

// ---------------------------------------------------------------------------
// MIFARE Classic commands
// ---------------------------------------------------------------------------

void test_authenticate_frames_differ_only_in_the_key_slot_byte() {
    const uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t frameA[8] = {};
    uint8_t frameB[8] = {};
    TEST_ASSERT_EQUAL_UINT16(
        8, Iso14443aFraming::buildAuthenticate(MifareKeySlot::KeyA, 4, keyA, frameA));
    TEST_ASSERT_EQUAL_UINT16(
        8, Iso14443aFraming::buildAuthenticate(MifareKeySlot::KeyB, 4, keyA, frameB));
    TEST_ASSERT_EQUAL_UINT8(0x60U, frameA[0]);
    TEST_ASSERT_EQUAL_UINT8(0x61U, frameB[0]);
    // Block number then the six key bytes, and nothing else: the reader appends
    // the CRC and runs Crypto1.
    TEST_ASSERT_EQUAL_UINT8(0x04U, frameA[1]);
    for (uint8_t i = 0; i < 6; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0xFFU, frameA[2 + i]);
        TEST_ASSERT_EQUAL_UINT8(0xFFU, frameB[2 + i]);
    }
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildAuthenticate(
                                     MifareKeySlot::KeyA, 4, nullptr, frameA));
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildAuthenticate(
                                     MifareKeySlot::KeyA, 4, keyA, nullptr));
}

void test_read_and_write_block_frames() {
    uint8_t frame[24] = {};
    TEST_ASSERT_EQUAL_UINT16(2, Iso14443aFraming::buildReadBlock(7, frame));
    TEST_ASSERT_EQUAL_UINT8(0x30U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x07U, frame[1]);

    uint8_t data[16];
    for (uint8_t i = 0; i < 16; ++i) {
        data[i] = static_cast<uint8_t>(0xA0U + i);
    }
    TEST_ASSERT_EQUAL_UINT16(18, Iso14443aFraming::buildWriteBlock(9, data, frame));
    TEST_ASSERT_EQUAL_UINT8(0xA0U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x09U, frame[1]);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(frame + 2, data, 16));
    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildWriteBlock(9, nullptr, frame));
}

void test_mifare_ack_and_nak_read_the_low_nibble() {
    const uint8_t ack[1] = {0x0A};
    const uint8_t nak[1] = {0x00};
    // The ACK/NACK contract is low-nibble: the PN532 hands back the single
    // response byte with the 4-bit code in the low nibble. A high-aligned 0xA0
    // is a transport that has not normalised yet and must NOT read as an ACK.
    const uint8_t ackHighAligned[1] = {0xA0};
    TEST_ASSERT_TRUE(Iso14443aFraming::isMifareAck(ack, 1));
    TEST_ASSERT_TRUE(Iso14443aFraming::isMifareNak(nak, 1));
    TEST_ASSERT_FALSE(Iso14443aFraming::isMifareAck(ackHighAligned, 1));
    TEST_ASSERT_FALSE(Iso14443aFraming::isMifareAck(nak, 1));
    TEST_ASSERT_FALSE(Iso14443aFraming::isMifareAck(ack, 0));
    TEST_ASSERT_FALSE(Iso14443aFraming::isMifareAck(nullptr, 1));
}

// ---------------------------------------------------------------------------
// NTAG / Ultralight commands
// ---------------------------------------------------------------------------

void test_ntag_command_frames() {
    uint8_t frame[16] = {};
    TEST_ASSERT_EQUAL_UINT16(2, Iso14443aFraming::buildNtagReadPage(0x2A, frame));
    TEST_ASSERT_EQUAL_UINT8(0x30U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x2AU, frame[1]);

    const uint8_t page[4] = {0x03, 0x11, 0xD1, 0x01};
    TEST_ASSERT_EQUAL_UINT16(6, Iso14443aFraming::buildNtagWritePage(0x04, page, frame));
    TEST_ASSERT_EQUAL_UINT8(0xA2U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x04U, frame[1]);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(frame + 2, page, 4));

    TEST_ASSERT_EQUAL_UINT16(1, Iso14443aFraming::buildNtagGetVersion(frame));
    TEST_ASSERT_EQUAL_UINT8(0x60U, frame[0]);

    const uint8_t password[4] = {0x11, 0x22, 0x33, 0x44};
    TEST_ASSERT_EQUAL_UINT16(5, Iso14443aFraming::buildNtagPwdAuth(password, frame));
    TEST_ASSERT_EQUAL_UINT8(0x1BU, frame[0]);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(frame + 1, password, 4));

    TEST_ASSERT_EQUAL_UINT16(0, Iso14443aFraming::buildNtagGetVersion(nullptr));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_crc_a_matches_the_catalogue_check_value);
    RUN_TEST(test_crc_a_agrees_with_an_independent_implementation);
    RUN_TEST(test_crc_a_ntag_read_frame_vector);
    RUN_TEST(test_crc_check_rejects_a_flipped_bit_and_a_short_frame);
    RUN_TEST(test_reqA_and_wupA_are_single_bytes);
    RUN_TEST(test_cascade_geometry_follows_the_uid_length);
    RUN_TEST(test_block_check_character_is_the_xor_of_its_four_bytes);
    RUN_TEST(test_select_for_a_four_byte_uid);
    RUN_TEST(test_select_for_a_seven_byte_uid_uses_the_cascade_tag);
    RUN_TEST(test_select_for_a_ten_byte_uid_uses_three_levels);
    RUN_TEST(test_select_rejects_a_level_past_the_uid);
    RUN_TEST(test_authenticate_frames_differ_only_in_the_key_slot_byte);
    RUN_TEST(test_read_and_write_block_frames);
    RUN_TEST(test_mifare_ack_and_nak_read_the_low_nibble);
    RUN_TEST(test_ntag_command_frames);
    return UNITY_END();
}
