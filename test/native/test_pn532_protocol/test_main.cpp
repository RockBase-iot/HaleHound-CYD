// Host suite for the PN532 host-controller frame codec.
//
// This is the layer the repository would otherwise inherit from the Adafruit
// library, where inDataExchange() collapses the PN532's status byte into a
// bool and merges "the card rejected this key" with "the I2C bus fell over".
// Key Check cannot report honestly behind an interface like that, so the codec
// is ours -- which means it has to be pinned, and pinned against something
// other than itself.
//
// Two anchors are used for that:
//
//   * a real PN532 1.6 reply to GetFirmwareVersion, reproduced byte for byte,
//     so a builder/decoder pair that agrees on a shared mistake still fails;
//   * the documented status code 0x14, which must map to "the card said no"
//     and never to a bus fault.

#include <unity.h>

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "drivers/Pn532Protocol.h"

namespace {

// Builds a PN532 -> host reply the way UM0701-02 §7.1 defines it. Used for the
// cases the captured frame does not cover; the captured frame below is what
// proves the builder and the decoder agree with the datasheet rather than with
// each other.
std::vector<uint8_t> replyFrame(uint8_t responseCode,
                               std::initializer_list<uint8_t> payload = {}) {
    std::vector<uint8_t> frame;
    const uint16_t len = static_cast<uint16_t>(payload.size() + 2U);
    frame.push_back(0x00);
    frame.push_back(0x00);
    frame.push_back(0xFF);
    frame.push_back(static_cast<uint8_t>(len));
    frame.push_back(static_cast<uint8_t>(~len + 1U));
    frame.push_back(kPn532Pn532ToHost);
    frame.push_back(responseCode);
    uint8_t sum = kPn532Pn532ToHost + responseCode;
    for (uint8_t byte : payload) {
        frame.push_back(byte);
        sum = static_cast<uint8_t>(sum + byte);
    }
    frame.push_back(static_cast<uint8_t>(~sum + 1U));
    frame.push_back(kPn532Postamble);
    return frame;
}

}  // namespace

// ---------------------------------------------------------------------------
// Host -> PN532
// ---------------------------------------------------------------------------

void test_a_host_frame_matches_the_documented_shape() {
    uint8_t out[64] = {};

    // GetFirmwareVersion: LEN 02, LCS FE, TFI D4, code 02, DCS 2A.
    const uint8_t getFirmware[1] = {kPn532CmdGetFirmwareVersion};
    TEST_ASSERT_EQUAL(9, pn532EncodeCommand(getFirmware, 1, out));
    const uint8_t expected[9] = {0x00, 0x00, 0xFF, 0x02, 0xFE,
                                 0xD4, 0x02, 0x2A, 0x00};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(out, expected, sizeof(expected)));

    // SAMConfiguration: four parameters, so LEN 05 and LCS FB.
    const uint8_t sam[4] = {kPn532CmdSamConfiguration, 0x01, 0x14, 0x01};
    TEST_ASSERT_EQUAL(12, pn532EncodeCommand(sam, sizeof(sam), out));
    const uint8_t expectedSam[12] = {0x00, 0x00, 0xFF, 0x05, 0xFB, 0xD4,
                                     0x14, 0x01, 0x14, 0x01, 0x02, 0x00};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(out, expectedSam, sizeof(expectedSam)));

    // Nothing to encode is not a frame.
    TEST_ASSERT_EQUAL(0, pn532EncodeCommand(nullptr, 1, out));
    TEST_ASSERT_EQUAL(0, pn532EncodeCommand(getFirmware, 0, out));
    TEST_ASSERT_EQUAL(0, pn532EncodeCommand(getFirmware, 1, nullptr));
}

// ---------------------------------------------------------------------------
// PN532 -> host
// ---------------------------------------------------------------------------

void test_a_captured_firmware_reply_decodes_to_the_documented_fields() {
    // A real PN532 1.6 answering GetFirmwareVersion: IC 0x32, version 1,
    // revision 6, support 7.
    const uint8_t captured[13] = {0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03,
                                  0x32, 0x01, 0x06, 0x07, 0xE8, 0x00};
    Pn532Frame frame;
    TEST_ASSERT_TRUE(pn532DecodeFrame(captured, sizeof(captured), frame));
    TEST_ASSERT_TRUE(frame.valid);
    TEST_ASSERT_EQUAL_UINT8(kPn532Pn532ToHost, frame.direction);
    TEST_ASSERT_EQUAL_UINT8(0x03U, frame.code);
    TEST_ASSERT_EQUAL_UINT8(4U, frame.length);

    Pn532FirmwareInfo info;
    TEST_ASSERT_TRUE(pn532ParseFirmwareVersion(frame, info));
    TEST_ASSERT_EQUAL_UINT8(0x32U, info.ic);
    TEST_ASSERT_EQUAL_UINT8(0x01U, info.version);
    TEST_ASSERT_EQUAL_UINT8(0x06U, info.revision);
    TEST_ASSERT_EQUAL_UINT8(0x07U, info.support);

    // The transport reads a fixed 64-byte buffer, so the same frame has to
    // decode with zero padding after the postamble.
    uint8_t padded[64] = {};
    std::memcpy(padded, captured, sizeof(captured));
    Pn532Frame paddedFrame;
    TEST_ASSERT_TRUE(pn532DecodeFrame(padded, sizeof(padded), paddedFrame));
    TEST_ASSERT_EQUAL_UINT8(0x32U, paddedFrame.data[0]);
}

void test_decoding_refuses_a_damaged_frame() {
    const uint8_t captured[13] = {0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03,
                                  0x32, 0x01, 0x06, 0x07, 0xE8, 0x00};
    Pn532Frame frame;

    uint8_t broken[13];
    std::memcpy(broken, captured, sizeof(broken));

    // A wrong LCS.
    broken[4] ^= 0x01U;
    TEST_ASSERT_FALSE(pn532DecodeFrame(broken, sizeof(broken), frame));
    std::memcpy(broken, captured, sizeof(broken));

    // A wrong DCS. This is the one that matters: a truncated read and a real
    // MIFARE NAK differ by a single byte, so a checksum that is not enforced
    // lets a short read masquerade as a card verdict.
    broken[11] ^= 0x01U;
    TEST_ASSERT_FALSE(pn532DecodeFrame(broken, sizeof(broken), frame));
    std::memcpy(broken, captured, sizeof(broken));

    // A wrong preamble byte.
    broken[2] = 0x00;
    TEST_ASSERT_FALSE(pn532DecodeFrame(broken, sizeof(broken), frame));
    std::memcpy(broken, captured, sizeof(broken));

    // Truncated: LEN claims six bytes that are not there.
    TEST_ASSERT_FALSE(pn532DecodeFrame(captured, 6, frame));

    // A frame claiming zero length carries no direction byte at all.
    const uint8_t empty[7] = {0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_FALSE(pn532DecodeFrame(empty, sizeof(empty), frame));

    TEST_ASSERT_FALSE(pn532DecodeFrame(nullptr, 13, frame));
}

void test_the_ack_frame_is_recognised_with_or_without_a_trailing_postamble() {
    TEST_ASSERT_EQUAL(6, kPn532AckLength);
    TEST_ASSERT_TRUE(pn532IsAckFrame(kPn532AckFrame, kPn532AckLength));

    // Some I2C reads include the postamble after the ACK.
    const uint8_t withPostamble[7] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00};
    TEST_ASSERT_TRUE(pn532IsAckFrame(withPostamble, sizeof(withPostamble)));

    // Anything after that means two frames arrived and only the first was
    // looked at, which must not pass as a clean ACK.
    const uint8_t ackThenFrame[8] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xD5, 0x03};
    TEST_ASSERT_FALSE(pn532IsAckFrame(ackThenFrame, sizeof(ackThenFrame)));

    const uint8_t corrupted[6] = {0x00, 0x00, 0xFF, 0x00, 0xFE, 0x00};
    TEST_ASSERT_FALSE(pn532IsAckFrame(corrupted, sizeof(corrupted)));
    TEST_ASSERT_FALSE(pn532IsAckFrame(kPn532AckFrame, 5));
    TEST_ASSERT_FALSE(pn532IsAckFrame(nullptr, kPn532AckLength));
}

// ---------------------------------------------------------------------------
// Status: the card's verdict versus the bus's
// ---------------------------------------------------------------------------

void test_the_status_mapping_separates_the_card_from_the_bus() {
    TEST_ASSERT_EQUAL(Pn532Status::MifareAuthError, pn532StatusFromByte(0x14));
    // The distinction the whole honest-reporting contract rests on.
    TEST_ASSERT_EQUAL(RfidStatus::AuthFailed,
                      pn532StatusToRfid(Pn532Status::MifareAuthError));

    TEST_ASSERT_EQUAL(RfidStatus::Ok, pn532StatusToRfid(Pn532Status::Ok));
    TEST_ASSERT_EQUAL(RfidStatus::Timeout, pn532StatusToRfid(Pn532Status::Timeout));
    TEST_ASSERT_EQUAL(RfidStatus::CrcError, pn532StatusToRfid(Pn532Status::CrcError));
    TEST_ASSERT_EQUAL(RfidStatus::CrcError, pn532StatusToRfid(Pn532Status::ParityError));
    TEST_ASSERT_EQUAL(RfidStatus::FrameError,
                      pn532StatusToRfid(Pn532Status::FramingError));
    TEST_ASSERT_EQUAL(RfidStatus::Collision,
                      pn532StatusToRfid(Pn532Status::AbnormalCollision));
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget,
                      pn532StatusToRfid(Pn532Status::TargetReleased));
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported,
                      pn532StatusToRfid(Pn532Status::InvalidParameter));
    TEST_ASSERT_EQUAL(RfidStatus::IoError, pn532StatusToRfid(Pn532Status::Unknown));

    // The wrong card answered the select, so the activation has to be redone.
    // That is a host problem, never a verdict on a key.
    TEST_ASSERT_EQUAL(RfidStatus::FrameError,
                      pn532StatusToRfid(Pn532Status::UidCheckByteWrong));

    // 0x08 is not a documented code, so it decodes to Unknown rather than to a
    // made-up cause -- and an unknown code must not read as a rejected key.
    TEST_ASSERT_EQUAL(Pn532Status::Unknown, pn532StatusFromByte(0x08));
    TEST_ASSERT_NOT_EQUAL(RfidStatus::AuthFailed,
                          pn532StatusToRfid(Pn532Status::Unknown));

    TEST_ASSERT_NOT_NULL(pn532StatusLabel(Pn532Status::MifareAuthError));
    TEST_ASSERT_NOT_NULL(pn532StatusLabel(Pn532Status::Unknown));
    TEST_ASSERT_EQUAL_STRING("ok", pn532StatusLabel(Pn532Status::Ok));
}

// ---------------------------------------------------------------------------
// Typed replies
// ---------------------------------------------------------------------------

void test_a_passive_target_reply_is_parsed_field_by_field() {
    const std::vector<uint8_t> reply =
        replyFrame(kPn532RespInListPassiveTarget, {0x01, 0x01, 0x04, 0x00, 0x08, 0x04,
                                                   0x04, 0x12, 0x34, 0x56});
    Pn532Frame frame;
    TEST_ASSERT_TRUE(pn532DecodeFrame(reply.data(), static_cast<uint16_t>(reply.size()), frame));
    TEST_ASSERT_EQUAL_UINT8(1, pn532PassiveTargetCount(frame));

    Pn532PassiveTarget target;
    TEST_ASSERT_TRUE(pn532ParsePassiveTarget(frame, target));
    TEST_ASSERT_EQUAL_UINT8(1, target.targetNumber);
    // ATQA arrives as the card sent it, least significant byte first.
    TEST_ASSERT_EQUAL_UINT16(0x0004, target.atqa);
    TEST_ASSERT_EQUAL_UINT8(0x08, target.sak);
    TEST_ASSERT_EQUAL_UINT8(4, target.uidLength);
    const uint8_t expectedUid[4] = {0x04, 0x12, 0x34, 0x56};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(target.uid, expectedUid, sizeof(expectedUid)));
    TEST_ASSERT_EQUAL_UINT8(0, target.atsLength);

    // The same frame with the ATS appended, as an ISO14443-4 card sends it.
    const std::vector<uint8_t> withAts =
        replyFrame(kPn532RespInListPassiveTarget, {0x01, 0x01, 0x04, 0x00, 0x08, 0x04,
                                                  0x04, 0x12, 0x34, 0x56, 0x05, 0x78,
                                                  0x33, 0x88, 0x00, 0x00});
    Pn532Frame atsFrame;
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(withAts.data(), static_cast<uint16_t>(withAts.size()), atsFrame));
    Pn532PassiveTarget atsTarget;
    TEST_ASSERT_TRUE(pn532ParsePassiveTarget(atsFrame, atsTarget));
    TEST_ASSERT_EQUAL_UINT8(5, atsTarget.atsLength);
    TEST_ASSERT_EQUAL_UINT8(0x78, atsTarget.ats[0]);
}

void test_an_empty_field_is_a_zero_but_an_unexpected_reply_is_not() {
    const std::vector<uint8_t> empty = replyFrame(kPn532RespInListPassiveTarget, {0x00});
    Pn532Frame frame;
    TEST_ASSERT_TRUE(pn532DecodeFrame(empty.data(), static_cast<uint16_t>(empty.size()), frame));
    TEST_ASSERT_EQUAL_UINT8(0, pn532PassiveTargetCount(frame));

    Pn532PassiveTarget target;
    TEST_ASSERT_FALSE(pn532ParsePassiveTarget(frame, target));

    // A reply that is not an InListPassiveTarget reply at all reports 0xFF, and
    // that is how the caller tells "no card" apart from "the reader answered
    // something I do not understand".
    const std::vector<uint8_t> other = replyFrame(kPn532RespInDataExchange, {0x01});
    Pn532Frame otherFrame;
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(other.data(), static_cast<uint16_t>(other.size()), otherFrame));
    TEST_ASSERT_EQUAL_UINT8(0xFFU, pn532PassiveTargetCount(otherFrame));
    TEST_ASSERT_FALSE(pn532ParsePassiveTarget(otherFrame, target));
}

void test_a_malformed_passive_target_reply_is_refused() {
    Pn532Frame frame;
    Pn532PassiveTarget target;

    // A UID length of zero cannot identify a card.
    const std::vector<uint8_t> noUid =
        replyFrame(kPn532RespInListPassiveTarget, {0x01, 0x01, 0x04, 0x00, 0x08, 0x00});
    TEST_ASSERT_TRUE(pn532DecodeFrame(noUid.data(), static_cast<uint16_t>(noUid.size()), frame));
    TEST_ASSERT_FALSE(pn532ParsePassiveTarget(frame, target));

    // Longer than any UID this firmware models.
    const std::vector<uint8_t> tooLong =
        replyFrame(kPn532RespInListPassiveTarget, {0x01, 0x01, 0x04, 0x00, 0x08, 0x0B,
                                                   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(tooLong.data(), static_cast<uint16_t>(tooLong.size()), frame));
    TEST_ASSERT_FALSE(pn532ParsePassiveTarget(frame, target));

    // Truncated before the UID even starts.
    const std::vector<uint8_t> shortReply =
        replyFrame(kPn532RespInListPassiveTarget, {0x01, 0x01, 0x04, 0x00, 0x08});
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(shortReply.data(), static_cast<uint16_t>(shortReply.size()), frame));
    TEST_ASSERT_FALSE(pn532ParsePassiveTarget(frame, target));

    // Two targets in the field is a collision the operator has to resolve, not
    // something for the parser to choose between.
    const std::vector<uint8_t> two =
        replyFrame(kPn532RespInListPassiveTarget, {0x02, 0x01, 0x04, 0x00, 0x08, 0x04,
                                                   0x04, 0x12, 0x34, 0x56});
    TEST_ASSERT_TRUE(pn532DecodeFrame(two.data(), static_cast<uint16_t>(two.size()), frame));
    TEST_ASSERT_EQUAL_UINT8(2, pn532PassiveTargetCount(frame));
    TEST_ASSERT_FALSE(pn532ParsePassiveTarget(frame, target));
}

void test_a_partial_ats_is_not_recorded_as_a_short_ats() {
    Pn532Frame frame;
    Pn532PassiveTarget target;

    // ATSLength says five, but only three bytes arrived: a truncated read, not
    // a card with a three-byte ATS.
    const std::vector<uint8_t> truncated =
        replyFrame(kPn532RespInListPassiveTarget, {0x01, 0x01, 0x04, 0x00, 0x08, 0x04,
                                                   0x04, 0x12, 0x34, 0x56, 0x05, 0x78,
                                                   0x33, 0x88});
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(truncated.data(), static_cast<uint16_t>(truncated.size()), frame));
    TEST_ASSERT_TRUE(pn532ParsePassiveTarget(frame, target));
    TEST_ASSERT_EQUAL_UINT8(4, target.uidLength);
    TEST_ASSERT_EQUAL_UINT8(0, target.atsLength);
}

void test_firmware_parsing_requires_the_firmware_response_code() {
    Pn532FirmwareInfo info;

    // Right payload, wrong response code: this is some other command's reply.
    const std::vector<uint8_t> wrongCode =
        replyFrame(kPn532RespInDataExchange, {0x32, 0x01, 0x06, 0x07});
    Pn532Frame frame;
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(wrongCode.data(), static_cast<uint16_t>(wrongCode.size()), frame));
    TEST_ASSERT_FALSE(pn532ParseFirmwareVersion(frame, info));

    // The firmware reply carries four bytes; two is not one of them.
    const std::vector<uint8_t> tooShort = replyFrame(0x03, {0x32, 0x01});
    Pn532Frame shortFrame;
    TEST_ASSERT_TRUE(
        pn532DecodeFrame(tooShort.data(), static_cast<uint16_t>(tooShort.size()), shortFrame));
    TEST_ASSERT_FALSE(pn532ParseFirmwareVersion(shortFrame, info));

    // An undecoded frame is never parsed.
    Pn532Frame invalid;
    TEST_ASSERT_FALSE(pn532ParseFirmwareVersion(invalid, info));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_a_host_frame_matches_the_documented_shape);
    RUN_TEST(test_a_captured_firmware_reply_decodes_to_the_documented_fields);
    RUN_TEST(test_decoding_refuses_a_damaged_frame);
    RUN_TEST(test_the_ack_frame_is_recognised_with_or_without_a_trailing_postamble);
    RUN_TEST(test_the_status_mapping_separates_the_card_from_the_bus);
    RUN_TEST(test_a_passive_target_reply_is_parsed_field_by_field);
    RUN_TEST(test_an_empty_field_is_a_zero_but_an_unexpected_reply_is_not);
    RUN_TEST(test_a_malformed_passive_target_reply_is_refused);
    RUN_TEST(test_a_partial_ats_is_not_recorded_as_a_short_ats);
    RUN_TEST(test_firmware_parsing_requires_the_firmware_response_code);
    return UNITY_END();
}
