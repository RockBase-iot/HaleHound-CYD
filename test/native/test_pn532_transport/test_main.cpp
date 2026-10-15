// Host suite for Pn532Transport -- the command sequencing over the I2C seam.
//
// What this pins:
//
//   1. bring-up wakes the part, probes the firmware (with a bounded retry),
//      configures SAM and the RF retry budget -- and fails cleanly when the
//      part never answers;
//   2. activation reports "no card" as NoTarget rather than as an error, and
//      returns the UID and geometry the part handed over;
//   3. transceive before a target is selected is NoTarget, not a card verdict;
//   4. a MIFARE authentication frame leaves with the PICC UID appended -- the
//      one documented deviation from UM0701-02, and the form proven against
//      real silicon -- while a data frame does not;
//   5. a reader status of 0x14 comes back as AuthFailed and an unrecognised
//      status does not. That is the honest-reporting contract: a rejected key
//      and a dead bus must never be the same answer.
//
// The link is scripted one chunk at a time, so a test reads as the reader's
// side of the conversation. link.writes holds the host frames the transport
// actually put on the bus, which is what makes (4) assertable.

#include <unity.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "FakePn532Link.h"
#include "drivers/Pn532Transport.h"
#include "features/rfid/MifareClassicModel.h"  // kMifareBlockSize

namespace {

constexpr uint8_t kMifareCommandAuthenticateA = 0x60;

// The host frame the transport wrote last, starting at the command byte.
//
// A host frame is preamble(3), LEN, LCS, TFI, then the command and its
// parameters, then DCS and the postamble -- so the command starts at offset 6
// and LEN counts the TFI plus everything after it.
std::vector<uint8_t> lastCommand(const FakePn532Link& link) {
    if (link.writes.empty()) {
        return {};
    }
    const std::vector<uint8_t>& frame = link.writes.back();
    if (frame.size() < 8) {
        return {};
    }
    const int commandLength = static_cast<int>(frame[3]) - 1;
    if (commandLength <= 0 || static_cast<size_t>(commandLength) + 6 > frame.size()) {
        return {};
    }
    return std::vector<uint8_t>(frame.begin() + 6, frame.begin() + 6 + commandLength);
}

// The reader's side of bring-up: firmware probe, SAMConfiguration,
// RFConfiguration.
void scriptBringUp(FakePn532Link& link) {
    link.expectAck();
    link.expectReplyFrame(0x03, {0x32, 0x01, 0x06, 0x07});  // PN532 1.6
    link.expectAck();
    link.expectReplyFrame(0x15, {0x00});  // SAMConfiguration, reader status ok
    link.expectAck();
    link.expectReplyFrame(0x33, {0x00});  // RFConfiguration, reader status ok
}

// A MIFARE Classic 1K in the field: one target, ATQA 0004, SAK 08, UID
// 04 12 34 56.
void scriptClassicTarget(FakePn532Link& link) {
    link.expectAck();
    link.expectReplyFrame(kPn532RespInListPassiveTarget,
                          {0x01, 0x01, 0x04, 0x00, 0x08, 0x04, 0x04, 0x12, 0x34, 0x56});
}

// Brings a transport all the way to "card selected", which is the state most of
// the transceive tests start from.
void bringUpWithTarget(FakePn532Link& link, Pn532Transport& transport) {
    scriptBringUp(link);
    TEST_ASSERT_TRUE(transport.begin());
    scriptClassicTarget(link);
    RfidTarget target;
    TEST_ASSERT_EQUAL(RfidStatus::Ok, transport.activateIso14443a(target, 200));
}

std::array<uint8_t, 8> authFrame() {
    return {{kMifareCommandAuthenticateA, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
}

}  // namespace

// ---------------------------------------------------------------------------
// Bring-up
// ---------------------------------------------------------------------------

void test_bring_up_probes_the_firmware_and_configures_the_part() {
    FakePn532Link link;
    scriptBringUp(link);
    Pn532Transport transport(link);

    TEST_ASSERT_TRUE(transport.begin());
    TEST_ASSERT_TRUE(transport.ready());
    TEST_ASSERT_EQUAL_UINT32(0x32010607UL, transport.firmwareVersion());
    TEST_ASSERT_EQUAL_STRING("PN532 1.6", transport.firmwareText());
    TEST_ASSERT_EQUAL_STRING("PN532 I2C", transport.name());

    // The first thing on the bus is the bare wake-up write, because the part
    // ignores the first real command otherwise and the init looks like a dead
    // board.
    TEST_ASSERT_EQUAL(4, link.writeCount);
    TEST_ASSERT_EQUAL(1, link.writes[0].size());
    TEST_ASSERT_EQUAL_UINT8(0x55U, link.writes[0][0]);

    // Then the firmware probe, then the two configuration commands.
    const int firmwareOffset = 6;
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdGetFirmwareVersion, link.writes[1][firmwareOffset]);
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdSamConfiguration, link.writes[2][firmwareOffset]);
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdRfConfiguration, link.writes[3][firmwareOffset]);

    // begin() is idempotent: a second call must not re-run the handshake.
    TEST_ASSERT_TRUE(transport.begin());
    TEST_ASSERT_EQUAL(4, link.writeCount);
}

void test_bring_up_fails_when_the_part_never_answers() {
    FakePn532Link link;  // nothing scripted, so ready() never goes true
    Pn532Transport transport(link);

    TEST_ASSERT_FALSE(transport.begin());
    TEST_ASSERT_FALSE(transport.ready());
    TEST_ASSERT_EQUAL_UINT32(0, transport.firmwareVersion());
    TEST_ASSERT_EQUAL_STRING("", transport.firmwareText());
    // One wake-up write plus the bounded number of firmware probes: the retry
    // is finite so a dead board fails instead of hanging the screen.
    TEST_ASSERT_EQUAL(6, link.writeCount);
}

void test_bring_up_fails_when_the_sam_configuration_is_refused() {
    FakePn532Link link;
    link.expectAck();
    link.expectReplyFrame(0x03, {0x32, 0x01, 0x06, 0x07});
    link.expectAck();
    link.expectReplyFrame(0x15, {0x27});  // "command not acceptable in this context"
    Pn532Transport transport(link);

    TEST_ASSERT_FALSE(transport.begin());
    TEST_ASSERT_FALSE(transport.ready());
}

// ---------------------------------------------------------------------------
// Activation
// ---------------------------------------------------------------------------

void test_activation_reports_an_empty_field_as_no_target() {
    FakePn532Link link;
    scriptBringUp(link);
    Pn532Transport transport(link);
    TEST_ASSERT_TRUE(transport.begin());

    link.expectAck();
    link.expectReplyFrame(kPn532RespInListPassiveTarget, {0x00});

    RfidTarget target;
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget, transport.activateIso14443a(target, 200));
    TEST_ASSERT_EQUAL(RfidTargetKind::None, target.kind);
    TEST_ASSERT_EQUAL_UINT8(0, target.uidLength);
}

void test_activation_returns_the_uid_and_geometry() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    // bringUpWithTarget asserted the status; re-derive the result to check the
    // fields it filled in.
    scriptClassicTarget(link);
    RfidTarget target;
    TEST_ASSERT_EQUAL(RfidStatus::Ok, transport.activateIso14443a(target, 200));
    TEST_ASSERT_EQUAL(RfidTargetKind::Iso14443A, target.kind);
    TEST_ASSERT_EQUAL_UINT8(4, target.uidLength);
    TEST_ASSERT_EQUAL_UINT16(0x0004, target.atqa);
    TEST_ASSERT_EQUAL_UINT8(0x08, target.sak);
    const uint8_t expectedUid[4] = {0x04, 0x12, 0x34, 0x56};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(target.uid.data(), expectedUid, sizeof(expectedUid)));
}

void test_activation_needs_an_initialised_part() {
    FakePn532Link link;
    Pn532Transport transport(link);

    RfidTarget target;
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, transport.activateIso14443a(target, 200));
    TEST_ASSERT_EQUAL(0, link.writeCount);
}

// ---------------------------------------------------------------------------
// Transceive
// ---------------------------------------------------------------------------

void test_transceive_before_a_target_is_selected_has_nowhere_to_go() {
    FakePn532Link link;
    scriptBringUp(link);
    Pn532Transport transport(link);
    TEST_ASSERT_TRUE(transport.begin());

    const uint8_t readBlock[2] = {0x30, 0x04};
    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget,
                      transport.transceive(readBlock, sizeof(readBlock), rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
    // Reporting this as a card verdict would be a lie, so nothing was sent.
    TEST_ASSERT_EQUAL(4, link.writeCount);
}

void test_a_zero_length_frame_is_refused() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::FrameError, transport.transceive(nullptr, 0, rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
}

void test_a_mifare_authentication_frame_carries_the_picc_uid() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    const std::array<uint8_t, 8> auth = authFrame();
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, {0x00});  // reader status ok, no payload

    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::Ok,
                      transport.transceive(auth.data(), auth.size(), rx, rxLength, 200));

    // InDataExchange, target number, the eight canonical auth bytes, then the
    // UID the part handed over at activation. This is the deviation from
    // UM0701-02 §7.3.9 that real silicon requires.
    const std::vector<uint8_t> commanded = lastCommand(link);
    TEST_ASSERT_EQUAL(14, commanded.size());
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdInDataExchange, commanded[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, commanded[1]);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(commanded.data() + 2, auth.data(), auth.size()));
    const uint8_t expectedUid[4] = {0x04, 0x12, 0x34, 0x56};
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(commanded.data() + 10, expectedUid,
                                         sizeof(expectedUid)));
}

void test_a_data_frame_leaves_without_the_uid() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    const uint8_t readBlock[2] = {0x30, 0x04};
    // The reader's reply is a status byte followed by the sixteen block bytes.
    const uint8_t block[kMifareBlockSize] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                             0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t reply[kMifareBlockSize + 1] = {0x00};
    std::memcpy(reply + 1, block, kMifareBlockSize);
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, reply, sizeof(reply));

    // The caller offers 32 bytes of room and the reply carries 16, which also
    // pins that the capacity survives into dataExchange(): clearing it on the
    // way in made every reply with content fail as Unsupported.
    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::Ok,
                      transport.transceive(readBlock, sizeof(readBlock), rx, rxLength, 200));
    // dataExchange() consumes the reader status byte, so what comes back is
    // the sixteen block bytes and nothing else.
    TEST_ASSERT_EQUAL_UINT16(kMifareBlockSize, rxLength);
    TEST_ASSERT_EQUAL_INT(0, std::memcmp(rx, block, kMifareBlockSize));

    // Two bytes of InDataExchange header plus the two card bytes, and nothing
    // else: with the UID appended this would be eight, so the length is what
    // proves the auth quirk stays confined to 0x60/0x61.
    const std::vector<uint8_t> commanded = lastCommand(link);
    TEST_ASSERT_EQUAL(4, commanded.size());
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdInDataExchange, commanded[0]);
    TEST_ASSERT_EQUAL_UINT8(0x30U, commanded[2]);
    TEST_ASSERT_EQUAL_UINT8(0x04U, commanded[3]);
}

void test_a_reply_too_large_for_the_caller_buffer_is_refused() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    const uint8_t readBlock[2] = {0x30, 0x04};
    const uint8_t block[kMifareBlockSize] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                             0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8_t reply[kMifareBlockSize + 1] = {0x00};
    std::memcpy(reply + 1, block, kMifareBlockSize);
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, reply, sizeof(reply));

    // Four bytes of room for a sixteen-byte block. Truncating silently would
    // hand the caller a short read that looks like a short card response.
    uint8_t rx[4] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported,
                      transport.transceive(readBlock, sizeof(readBlock), rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
}

// ---------------------------------------------------------------------------
// Honest reporting: the card's verdict versus the bus's
// ---------------------------------------------------------------------------

void test_a_rejected_key_is_not_reported_as_a_bus_fault() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    const std::array<uint8_t, 8> auth = authFrame();
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, {0x14});  // MIFARE authentication error

    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::AuthFailed,
                      transport.transceive(auth.data(), auth.size(), rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
    TEST_ASSERT_EQUAL(Pn532Status::MifareAuthError, transport.lastStatus());

    // An unrecognised reader status is a fault, and must give a different
    // answer. If these two ever collapse into one status, Key Check starts
    // reporting a dead bus as a card that protects its keys.
    link.expectAck();
    link.expectReplyFrame(kPn532RespInDataExchange, {0x08});
    rxLength = sizeof(rx);
    const RfidStatus fault =
        transport.transceive(auth.data(), auth.size(), rx, rxLength, 200);
    TEST_ASSERT_EQUAL(RfidStatus::IoError, fault);
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
    TEST_ASSERT_NOT_EQUAL(RfidStatus::AuthFailed, fault);
}

void test_a_reply_that_is_not_this_command_reply_is_a_frame_error() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    // A read is answered by InDataExchange (0x41). An InListPassiveTarget reply
    // here means a previous exchange was abandoned mid-read, which is a host
    // problem rather than anything the card said.
    const uint8_t readBlock[2] = {0x30, 0x04};
    link.expectAck();
    link.expectReplyFrame(kPn532RespInListPassiveTarget, {0x00});

    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::FrameError,
                      transport.transceive(readBlock, sizeof(readBlock), rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
}

void test_a_damaged_reply_is_a_frame_error_not_a_card_verdict() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    // A correct InDataExchange reply with one DCS bit flipped.
    const uint8_t corrupted[10] = {0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD5,
                                   0x41, 0x00, 0xEB, 0x00};
    const uint8_t readBlock[2] = {0x30, 0x04};
    link.expectAck();
    link.expectReply({0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD5, 0x41, 0x00, 0xEB, 0x00});

    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::FrameError,
                      transport.transceive(readBlock, sizeof(readBlock), rx, rxLength, 200));
    TEST_ASSERT_EQUAL_UINT16(0, rxLength);
}

// ---------------------------------------------------------------------------
// Release and field control
// ---------------------------------------------------------------------------

void test_release_forgets_the_target() {
    FakePn532Link link;
    Pn532Transport transport(link);
    bringUpWithTarget(link, transport);

    link.expectAck();
    link.expectReplyFrame(0x53, {0x00});  // InRelease reply
    TEST_ASSERT_EQUAL(RfidStatus::Ok, transport.releaseTarget());
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdInRelease, lastCommand(link)[0]);

    // The UID goes with the target number: it is spliced into the next
    // authentication frame, so a stale one would be sent to a different card.
    const uint8_t readBlock[2] = {0x30, 0x04};
    uint8_t rx[32] = {};
    uint16_t rxLength = sizeof(rx);
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget,
                      transport.transceive(readBlock, sizeof(readBlock), rx, rxLength, 200));

    // Releasing again reports there is nothing to release.
    TEST_ASSERT_EQUAL(RfidStatus::NoTarget, transport.releaseTarget());
}

void test_field_control_needs_an_initialised_part() {
    FakePn532Link link;
    Pn532Transport transport(link);

    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, transport.fieldOn());
    TEST_ASSERT_EQUAL(RfidStatus::Unsupported, transport.fieldOff());
    TEST_ASSERT_EQUAL(0, link.writeCount);
}

void test_field_control_switches_the_field_via_rf_configuration() {
    FakePn532Link link;
    scriptBringUp(link);
    Pn532Transport transport(link);
    TEST_ASSERT_TRUE(transport.begin());

    link.expectAck();
    link.expectReplyFrame(0x33, {0x00});
    TEST_ASSERT_EQUAL(RfidStatus::Ok, transport.fieldOn());
    const std::vector<uint8_t> on = lastCommand(link);
    TEST_ASSERT_EQUAL(3, on.size());
    TEST_ASSERT_EQUAL_UINT8(kPn532CmdRfConfiguration, on[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, on[1]);  // RFConfiguration item: the field
    TEST_ASSERT_EQUAL_UINT8(0x01U, on[2]);  // on

    link.expectAck();
    link.expectReplyFrame(0x33, {0x00});
    TEST_ASSERT_EQUAL(RfidStatus::Ok, transport.fieldOff());
    const std::vector<uint8_t> off = lastCommand(link);
    TEST_ASSERT_EQUAL(3, off.size());
    TEST_ASSERT_EQUAL_UINT8(0x00U, off[2]);  // off
}

// ---------------------------------------------------------------------------
// Capabilities
// ---------------------------------------------------------------------------

void test_capabilities_do_not_claim_what_the_driver_cannot_do() {
    FakePn532Link link;
    Pn532Transport transport(link);
    const RfidTransportCaps caps = transport.caps();

    TEST_ASSERT_TRUE(caps.iso14443a);
    // The silicon can do B and FeliCa, but this driver only builds type A
    // frames -- claiming B would let a screen offer a mode that cannot work.
    TEST_ASSERT_FALSE(caps.iso14443b);
    TEST_ASSERT_FALSE(caps.iso15693);
    TEST_ASSERT_TRUE(caps.isoDep);
    TEST_ASSERT_TRUE(caps.mifareClassic);
    TEST_ASSERT_TRUE(caps.mifareUltralight);
    TEST_ASSERT_TRUE(caps.ntag);
    // Not supported by the PN532 silicon at all.
    TEST_ASSERT_FALSE(caps.iclass);
    TEST_ASSERT_TRUE(caps.rawTransceive);
    // InListPassiveTarget runs the whole of ISO14443-3 inside the part.
    TEST_ASSERT_TRUE(caps.siliconAnticollision);
    TEST_ASSERT_FALSE(caps.lfSupport);
    TEST_ASSERT_FALSE(caps.mfkey32NonceCapture);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_bring_up_probes_the_firmware_and_configures_the_part);
    RUN_TEST(test_bring_up_fails_when_the_part_never_answers);
    RUN_TEST(test_bring_up_fails_when_the_sam_configuration_is_refused);
    RUN_TEST(test_activation_reports_an_empty_field_as_no_target);
    RUN_TEST(test_activation_returns_the_uid_and_geometry);
    RUN_TEST(test_activation_needs_an_initialised_part);
    RUN_TEST(test_transceive_before_a_target_is_selected_has_nowhere_to_go);
    RUN_TEST(test_a_zero_length_frame_is_refused);
    RUN_TEST(test_a_mifare_authentication_frame_carries_the_picc_uid);
    RUN_TEST(test_a_data_frame_leaves_without_the_uid);
    RUN_TEST(test_a_reply_too_large_for_the_caller_buffer_is_refused);
    RUN_TEST(test_a_rejected_key_is_not_reported_as_a_bus_fault);
    RUN_TEST(test_a_reply_that_is_not_this_command_reply_is_a_frame_error);
    RUN_TEST(test_a_damaged_reply_is_a_frame_error_not_a_card_verdict);
    RUN_TEST(test_release_forgets_the_target);
    RUN_TEST(test_field_control_needs_an_initialised_part);
    RUN_TEST(test_field_control_switches_the_field_via_rf_configuration);
    RUN_TEST(test_capabilities_do_not_claim_what_the_driver_cannot_do);
    return UNITY_END();
}
