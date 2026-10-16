// Chameleon Ultra over BLE, driven against a scripted Ultra.
//
// The protocol itself is verified separately in test_chameleon_frame. What this
// suite covers is the seam: that a reply which arrives in pieces is reassembled
// rather than half-read, that a reply to somebody else's command is not accepted
// as this one's answer, that the 32-byte slot table and the 16-byte enabled map
// are decoded into the model's slots with the firmware's own tag numbers, and
// that the device address is reported as the static random address the top two
// bits say it is. Those are the parts that look right on the bench and are not.

#include <unity.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "FakeBleCentral.h"
#include "features/ble/ChameleonFrame.h"
#include "features/ble/ChameleonUltraClient.h"
#include "features/rfid/ChameleonUltraModel.h"

namespace {

const char* kUnitAddress = "E0:11:22:33:44:55";

// Builds a reply frame the way the firmware would: status in the header, both
// LRCs recomputed over the final bytes. No assertion macro here: they expand to
// a bare `return;`, which a value-returning function cannot contain. Encoding
// failure shows up as an empty reply, which the client reports as a timeout.
std::vector<uint8_t> replyFrame(uint16_t command, const std::vector<uint8_t>& payload,
                                uint16_t status = 0) {
    std::vector<uint8_t> wire(kChameleonMaxFrameBytes, 0);
    const std::size_t written =
        encodeChameleonFrame(command, payload.empty() ? nullptr : payload.data(),
                             static_cast<uint16_t>(payload.size()), wire.data(), wire.size());
    wire[4] = static_cast<uint8_t>(status >> 8);
    wire[5] = static_cast<uint8_t>(status & 0xFF);
    wire[8] = chameleonLrc(wire.data() + 2, 6);
    wire.resize(written);
    return wire;
}

std::vector<uint8_t> replyFrame(uint16_t command, std::initializer_list<uint8_t> payload,
                                uint16_t status = 0) {
    return replyFrame(command, std::vector<uint8_t>(payload), status);
}

// A u16 field, big-endian, appended to a growing payload.
void pushU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

BleCharacteristicRef rx() {
    BleCharacteristicRef ref;
    ref.service = ChameleonUltraClient::serviceUuid();
    ref.characteristic = ChameleonUltraClient::rxUuid();
    return ref;
}

BleCharacteristicRef tx() {
    BleCharacteristicRef ref;
    ref.service = ChameleonUltraClient::serviceUuid();
    ref.characteristic = ChameleonUltraClient::txUuid();
    return ref;
}

// An Ultra: advertises and exposes the Nordic UART service, so a client that
// looks for the service rather than the name finds it.
void scriptUltraAdvert(FakeBleCentral& fake, const char* address = kUnitAddress,
                       const char* name = "ChameleonUltra") {
    BleAdvertisement advertisement;
    advertisement.address = FakeBleCentral::parseAddress(address);
    advertisement.address.type = BleAddressType::Random;
    advertisement.rssi = -45;
    advertisement.connectable = true;
    advertisement.serviceCount = 1;
    advertisement.serviceUuids[0] = ChameleonUltraClient::serviceUuid();
    if (name != nullptr) {
        std::snprintf(advertisement.name, sizeof(advertisement.name), "%s", name);
    }
    fake.addAdvertisement(advertisement);
}

void scriptUltraServices(FakeBleCentral& fake) {
    TEST_ASSERT_NOT_NULL(fake.addService(ChameleonUltraClient::serviceUuid()));
    TEST_ASSERT_NOT_NULL(fake.addCharacteristic(
        0, ChameleonUltraClient::rxUuid(), kBlePropWrite | kBlePropWriteNoResponse));
    TEST_ASSERT_NOT_NULL(
        fake.addCharacteristic(0, ChameleonUltraClient::txUuid(), kBlePropNotify));
}

// The whole link set up: scan script, service tree, connect.
void scriptConnectedUltra(FakeBleCentral& fake) {
    scriptUltraAdvert(fake);
    scriptUltraServices(fake);
}

}  // namespace

// ─── The service identity ──────────────────────────────────────────────────

void test_the_command_link_is_the_128_bit_nordic_uart_service() {
    // Nordic's documentation calls this service "16-bit offset 0x0001". It is
    // not 0x0001 -- and a client built on that reading would write to the wrong
    // characteristic and look confident doing it.
    char service[37] = {};
    TEST_ASSERT_TRUE(ChameleonUltraClient::serviceUuid().write(service, sizeof(service)));
    TEST_ASSERT_EQUAL_STRING("6E400001-B5A3-F393-E0A9-E50E24DCCA9E", service);
    TEST_ASSERT_FALSE(ChameleonUltraClient::serviceUuid().isShort());

    char rxText[37] = {};
    TEST_ASSERT_TRUE(ChameleonUltraClient::rxUuid().write(rxText, sizeof(rxText)));
    TEST_ASSERT_EQUAL_STRING("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", rxText);

    char txText[37] = {};
    TEST_ASSERT_TRUE(ChameleonUltraClient::txUuid().write(txText, sizeof(txText)));
    TEST_ASSERT_EQUAL_STRING("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", txText);
}

// ─── Finding and adopting a unit ───────────────────────────────────────────

void test_a_scan_keeps_only_connectable_peers_that_advertise_the_uart_service() {
    FakeBleCentral fake;
    scriptUltraAdvert(fake);

    // A device advertising a different service. Identification is by service,
    // not by name, so a same-named stranger is not a Chameleon.
    BleAdvertisement stranger;
    stranger.address = FakeBleCentral::parseAddress("AA:00:00:00:00:02");
    stranger.connectable = true;
    stranger.serviceCount = 1;
    stranger.serviceUuids[0] = BleUuid::fromShort(0x180F);
    std::snprintf(stranger.name, sizeof(stranger.name), "ChameleonUltra");
    fake.addAdvertisement(stranger);

    // A non-connectable beacon that does advertise it: nothing to open.
    BleAdvertisement beacon;
    beacon.address = FakeBleCentral::parseAddress("AA:00:00:00:00:03");
    beacon.connectable = false;
    beacon.serviceCount = 1;
    beacon.serviceUuids[0] = ChameleonUltraClient::serviceUuid();
    fake.addAdvertisement(beacon);

    ChameleonUltraClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scanForUnits(false));
    TEST_ASSERT_EQUAL_UINT8(1, client.unitCount());
    TEST_ASSERT_EQUAL_STRING("ChameleonUltra", client.unitNameAt(0).c_str());
    TEST_ASSERT_EQUAL_STRING("", client.unitNameAt(1).c_str());

    // The address bytes survive the scan with their type intact.
    BleAddress recovered;
    TEST_ASSERT_TRUE(client.unitAddressAt(0, recovered));
    char text[18] = {};
    TEST_ASSERT_TRUE(recovered.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING(kUnitAddress, text);
    TEST_ASSERT_TRUE(recovered.type == BleAddressType::Random);
    TEST_ASSERT_FALSE(client.unitAddressAt(1, recovered));
}

void test_connecting_exposes_the_command_link() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scanForUnits(false));

    BleAddress unit;
    TEST_ASSERT_TRUE(client.unitAddressAt(0, unit));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.connect(unit, 1000));
    TEST_ASSERT_TRUE(client.connected());
    TEST_ASSERT_TRUE(client.linkReady());
    // Subscribed to TX: without the subscription no reply can ever arrive, and
    // the failure would look like a silent device.
    TEST_ASSERT_EQUAL_UINT8(1, fake.subscriptionCount());
}

void test_a_peer_without_the_uart_service_is_not_a_connection() {
    FakeBleCentral fake;
    scriptUltraAdvert(fake);
    // A tree with no Nordic UART service in it.
    fake.addService(0x180F);
    fake.addCharacteristic(0, 0x2A19, kBlePropRead);

    ChameleonUltraClient client(fake);
    client.begin();
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scanForUnits(false));
    BleAddress unit;
    client.unitAddressAt(0, unit);

    TEST_ASSERT_EQUAL(BleStatus::NoService, client.connect(unit, 1000));
    TEST_ASSERT_FALSE(client.linkReady());
    // A half-open link would report success and then time out on the first
    // command; the connect is rolled back instead.
    TEST_ASSERT_FALSE(fake.connected());
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());
}

void test_auto_connect_adopts_the_first_unit_and_records_it() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    ChameleonSession session;
    session.begin();
    TEST_ASSERT_TRUE(session.state() == ChameleonState::Scanning);

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.autoConnect(session, 1000));
    TEST_ASSERT_TRUE(client.connected());
    // The model's own policy recorded the peer, so the panel and the link agree
    // about which unit is held.
    TEST_ASSERT_TRUE(session.state() == ChameleonState::Ready);
    TEST_ASSERT_EQUAL_STRING(kUnitAddress, session.address().c_str());
}

void test_auto_connect_does_not_steal_a_unit_already_held() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    ChameleonSession session;
    session.begin();
    TEST_ASSERT_TRUE(session.onDeviceFound("11:22:33:44:55:66"));

    TEST_ASSERT_EQUAL(BleStatus::Busy, client.autoConnect(session, 1000));
    // Nothing was scanned or connected: the held unit is the session's.
    TEST_ASSERT_FALSE(client.connected());
    TEST_ASSERT_EQUAL(0, fake.startScanCalls);
    TEST_ASSERT_EQUAL_STRING("11:22:33:44:55:66", session.address().c_str());
}

void test_auto_connect_with_no_units_is_not_found() {
    FakeBleCentral fake;
    ChameleonUltraClient client(fake);
    client.begin();
    ChameleonSession session;
    session.begin();

    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.autoConnect(session, 1000));
    TEST_ASSERT_FALSE(client.connected());
    // The scan was still stopped, so the controller is not left scanning.
    TEST_ASSERT_FALSE(fake.scanning());
}

// ─── A command and its reply ────────────────────────────────────────────────

void test_a_command_returns_its_own_reply() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1033, {0x00}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    uint8_t model = 0xFF;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.readDeviceModel(model, 1000));
    TEST_ASSERT_EQUAL_HEX8(0x00, model);  // 0 = Ultra

    // The frame that went out is a real frame, not just bytes: the peer would
    // reject anything else.
    const std::vector<uint8_t>& sent = fake.lastWrite().data;
    ChameleonFrame echoed;
    TEST_ASSERT_TRUE(decodeChameleonFrame(sent.data(), sent.size(), echoed) > 0);
    TEST_ASSERT_EQUAL_HEX16(1033, echoed.command);
    TEST_ASSERT_EQUAL_UINT16(0, echoed.length);
    // And it went to RX, the write side of the UART service.
    TEST_ASSERT_TRUE(fake.lastWrite().ref == rx());
}

void test_a_reply_for_a_different_command_is_not_accepted() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    // The peer answers, but with somebody else's command number.
    fake.queueReply(rx(), tx(), replyFrame(1000, {0x01, 0x02}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    uint8_t model = 0;
    TEST_ASSERT_EQUAL(BleStatus::Timeout, client.readDeviceModel(model, 1000));
    // Counted, so "the device answered something else" is visible rather than
    // reported as silence.
    TEST_ASSERT_EQUAL_UINT32(1, client.strayFrames());
}

void test_no_reply_at_all_is_a_timeout() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    uint8_t model = 0;
    TEST_ASSERT_EQUAL(BleStatus::Timeout, client.readDeviceModel(model, 1000));
    TEST_ASSERT_EQUAL_UINT32(0, client.strayFrames());
    TEST_ASSERT_EQUAL(1, fake.writeCalls);  // the request *was* sent
}

void test_a_command_before_the_link_is_up_is_disconnected() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    ChameleonFrame reply;
    TEST_ASSERT_EQUAL(BleStatus::Disconnected,
                      client.command(1019, nullptr, 0, reply, 1000));
    TEST_ASSERT_EQUAL(0, fake.writeCalls);
}

void test_a_reply_longer_than_the_mtu_is_reassembled_from_notifications() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    // The real drive: a 32-byte slot table comes back in 20-byte notifications,
    // so it arrives as two pieces with the header in the first and the tail in
    // the second. Reassembling across notifications is the whole reason the
    // receiver is byte-fed.
    std::vector<uint8_t> info;
    for (uint8_t slot = 0; slot < kChameleonSlotCount; ++slot) {
        pushU16(info, 1001);  // hf: MIFARE 1K
        pushU16(info, 0);     // lf: nothing
    }
    const std::vector<uint8_t> whole = replyFrame(1019, info);

    std::vector<uint8_t> firstChunk(whole.begin(), whole.begin() + 13);
    std::vector<uint8_t> secondChunk(whole.begin() + 13, whole.end());
    fake.queueExchange(rx(), tx(), {firstChunk, secondChunk});

    std::vector<uint8_t> enabled(kChameleonSlotCount * 2U, 0x00);
    enabled[0] = 0x01;  // slot 0 enabled on HF
    fake.queueReply(rx(), tx(), replyFrame(1023, enabled));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    ChameleonSlot slots[kChameleonSlotCount];
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.readSlots(slots, 1000));

    for (uint8_t slot = 0; slot < kChameleonSlotCount; ++slot) {
        TEST_ASSERT_EQUAL_UINT8(slot, slots[slot].index);
        TEST_ASSERT_EQUAL_HEX8(static_cast<unsigned>(ChameleonSlotKind::MifareClassic1K),
                              static_cast<unsigned>(slots[slot].kind));
        TEST_ASSERT_TRUE(slots[slot].loaded);
    }
    TEST_ASSERT_TRUE(slots[0].enabled);
    TEST_ASSERT_FALSE(slots[1].enabled);
    TEST_ASSERT_EQUAL_UINT32(0, client.rejectedFrames());
}

void test_a_reply_of_the_wrong_size_is_not_used_as_the_answer() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    // The slot table is a fixed 32 bytes. A 16-byte answer is not a slot table,
    // and reading it as one would put eight invented slots on the panel.
    fake.queueReply(rx(), tx(), replyFrame(1019, std::vector<uint8_t>(16, 0x01)));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    ChameleonSlot slots[kChameleonSlotCount];
    TEST_ASSERT_EQUAL(BleStatus::IoError, client.readSlots(slots, 1000));
}

void test_a_failed_status_is_an_answer_and_not_a_link_error() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    // The handler refused. The frame is well formed; the status says no. That is
    // a fact about the device, not about the link.
    fake.queueReply(rx(), tx(), replyFrame(1003, {}, 0x0007));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    TEST_ASSERT_EQUAL(BleStatus::NotPermitted, client.setActiveSlot(2, 1000));
    // And no stray was counted: the reply was this command's.
    TEST_ASSERT_EQUAL_UINT32(0, client.strayFrames());
}

// ─── Device identity ───────────────────────────────────────────────────────

void test_the_device_address_is_a_static_random_address_not_an_untyped_number() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(),
                    replyFrame(1012, {0xF0, 0x11, 0x22, 0x33, 0x44, 0x55}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    BleAddress address;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.readDeviceAddress(address, 1000));
    char text[18] = {};
    TEST_ASSERT_TRUE(address.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("F0:11:22:33:44:55", text);
    // The firmware forces the top two bits to 0b11, which is what makes this a
    // BLE static random address. Reporting Unknown would throw that away, and the
    // type is part of the identity.
    TEST_ASSERT_TRUE(address.type == BleAddressType::Random);

    std::string printable;
    fake.queueReply(rx(), tx(), replyFrame(1012, {0xF0, 0x11, 0x22, 0x33, 0x44, 0x55}));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.readDeviceAddressText(printable, 1000));
    TEST_ASSERT_EQUAL_STRING("F0:11:22:33:44:55", printable.c_str());
}

void test_a_device_address_of_the_wrong_length_is_refused() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1012, {0x11, 0x22, 0x33}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    BleAddress address;
    TEST_ASSERT_EQUAL(BleStatus::IoError, client.readDeviceAddress(address, 1000));
}

void test_the_app_version_reads_major_then_minor() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1000, {0x02, 0x07}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    uint8_t major = 0;
    uint8_t minor = 0;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.readAppVersion(major, minor, 1000));
    TEST_ASSERT_EQUAL_HEX8(0x02, major);
    TEST_ASSERT_EQUAL_HEX8(0x07, minor);
}

// ─── The tag-type mapping ──────────────────────────────────────────────────

void test_the_slot_kinds_come_from_the_firmware_tag_numbers() {
    // tag_base_type.h: MIFARE 1024 is 1001 and 4096 is 1003.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(1001, 0) ==
                     ChameleonSlotKind::MifareClassic1K);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(1003, 0) ==
                     ChameleonSlotKind::MifareClassic4K);
    // NTAG 213/215/216 share the model's 21x kind.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(1100, 0) == ChameleonSlotKind::Ntag21x);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(1102, 0) == ChameleonSlotKind::Ntag21x);
    // The MF0 chips are the model's Ultralight.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(1103, 0) ==
                     ChameleonSlotKind::MifareUltralight);
    // LF types are only consulted when the HF side says nothing.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(0, 100) == ChameleonSlotKind::Em4100);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(0, 200) == ChameleonSlotKind::HidProx);
    // An LF tag number sitting in the HF field is not an HF tag: the firmware
    // numbers the two bands in separate ranges, so naming it would be a guess.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(100, 0) == ChameleonSlotKind::Unknown);
    // Both undefined is an empty slot, not an unrecognised one.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(0, 0) == ChameleonSlotKind::Empty);
}

void test_a_tag_this_build_cannot_name_is_unrecognised_not_guessed() {
    // MIFARE 2048 (1002) has no kind in the model. Calling it a 1K because that
    // is the nearest label available would put a wrong card name on the panel.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(1002, 0) == ChameleonSlotKind::Unknown);
    // ISO14443-4 and SEOS likewise.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(3000, 0) == ChameleonSlotKind::Unknown);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(3001, 0) == ChameleonSlotKind::Unknown);
    // And so do the LF formats the model has no kind for.
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(0, 150) == ChameleonSlotKind::Unknown);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(0, 310) == ChameleonSlotKind::Unknown);
}

void test_an_old_style_tag_number_is_migrated_before_it_is_mapped() {
    // tag_base_type.h keeps the pre-rename enum at 1..8 and says slots still
    // holding one need migrating first. Without that, a slot with a perfectly
    // nameable MIFARE 1K would show as Unrecognised.
    TEST_ASSERT_EQUAL_HEX16(1001, ChameleonUltraClient::normaliseTagType(3));
    TEST_ASSERT_EQUAL_HEX16(100, ChameleonUltraClient::normaliseTagType(1));
    TEST_ASSERT_EQUAL_HEX16(1100, ChameleonUltraClient::normaliseTagType(6));
    // A current number passes through untouched.
    TEST_ASSERT_EQUAL_HEX16(1001, ChameleonUltraClient::normaliseTagType(1001));
    TEST_ASSERT_EQUAL_HEX16(0, ChameleonUltraClient::normaliseTagType(0));

    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(3, 0) == ChameleonSlotKind::MifareClassic1K);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(0, 1) == ChameleonSlotKind::Em4100);
    TEST_ASSERT_TRUE(ChameleonUltraClient::slotKindFor(6, 0) == ChameleonSlotKind::Ntag21x);
}

void test_a_slot_table_of_old_numbers_still_names_cards() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    std::vector<uint8_t> info;
    pushU16(info, 3);  // slot 0 hf: OLD MIFARE 1024
    pushU16(info, 1);  // slot 0 lf: OLD EM410X -- both loaded
    for (uint8_t slot = 1; slot < kChameleonSlotCount; ++slot) {
        pushU16(info, 0);
        pushU16(info, 0);
    }
    fake.queueReply(rx(), tx(), replyFrame(1019, info));
    fake.queueReply(rx(), tx(), replyFrame(1023, std::vector<uint8_t>(16, 0x00)));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    ChameleonSlot slots[kChameleonSlotCount];
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.readSlots(slots, 1000));
    TEST_ASSERT_TRUE(slots[0].loaded);
    TEST_ASSERT_TRUE(slots[0].kind == ChameleonSlotKind::MifareClassic1K);
    TEST_ASSERT_FALSE(slots[1].loaded);
    TEST_ASSERT_TRUE(slots[1].kind == ChameleonSlotKind::Empty);
}

// ─── Slot writes ───────────────────────────────────────────────────────────

void test_a_nickname_goes_out_as_slot_sense_and_name() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1007, {}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    TEST_ASSERT_EQUAL(BleStatus::Ok,
                      client.setSlotNickname(3, ChameleonSense::Hf, "My MIFARE", 1000));

    // The wire form is slot | sense | name with no terminator, as documented.
    const std::vector<uint8_t>& sent = fake.lastWrite().data;
    ChameleonFrame frame;
    TEST_ASSERT_TRUE(decodeChameleonFrame(sent.data(), sent.size(), frame) > 0);
    TEST_ASSERT_EQUAL_HEX16(1007, frame.command);
    TEST_ASSERT_EQUAL_UINT16(2U + 9U, frame.length);
    TEST_ASSERT_EQUAL_HEX8(0x03, frame.data[0]);
    TEST_ASSERT_EQUAL_HEX8(static_cast<uint8_t>(ChameleonSense::Hf), frame.data[1]);
    TEST_ASSERT_EQUAL_STRING("My MIFARE",
                             std::string(reinterpret_cast<const char*>(frame.data + 2), 9).c_str());
}

void test_a_nickname_that_does_not_fit_the_field_is_refused() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    // The firmware field is 32 bytes with no terminator; a 33-byte name would be
    // cut by whoever is closest to the wire.
    const std::string tooLong(33, 'x');
    TEST_ASSERT_EQUAL(BleStatus::TooLong,
                      client.setSlotNickname(0, ChameleonSense::Hf, tooLong.c_str(), 1000));
    // Nothing was sent, so the device never stored half a name.
    TEST_ASSERT_EQUAL(0, fake.writeCalls);
}

void test_a_slot_with_no_nickname_is_not_found_not_an_empty_string() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    // The firmware's documented answer when nothing is recorded in flash.
    fake.queueReply(rx(), tx(), replyFrame(1008, {}, 0x0002));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    std::string nickname = "stale";
    TEST_ASSERT_EQUAL(BleStatus::NotFound,
                      client.readSlotNickname(1, ChameleonSense::Hf, nickname, 1000));
    // And the caller's buffer was cleared: an empty slot is not an empty name,
    // and leaving the old text in place would show it as this slot's.
    TEST_ASSERT_EQUAL_STRING("", nickname.c_str());
}

void test_a_recorded_nickname_comes_back_verbatim() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    const std::string stored = "Office 1K";
    fake.queueReply(rx(), tx(),
                    replyFrame(1008, std::vector<uint8_t>(stored.begin(), stored.end())));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    std::string nickname;
    TEST_ASSERT_EQUAL(BleStatus::Ok,
                      client.readSlotNickname(2, ChameleonSense::Hf, nickname, 1000));
    TEST_ASSERT_EQUAL_STRING("Office 1K", nickname.c_str());
}

void test_a_slot_index_outside_the_eight_is_refused_before_anything_is_sent() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.setActiveSlot(8, 1000));
    TEST_ASSERT_EQUAL(BleStatus::NotFound,
                      client.setSlotNickname(8, ChameleonSense::Hf, "x", 1000));
    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.setSlotEnabled(8, ChameleonSense::Hf, true, 1000));
    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.setSlotTagType(8, 1001, 1000));
    std::string nickname;
    TEST_ASSERT_EQUAL(BleStatus::NotFound,
                      client.readSlotNickname(9, ChameleonSense::Hf, nickname, 1000));
    TEST_ASSERT_EQUAL(0, fake.writeCalls);

    // Sense::None is not a band, so it is not a valid sense selector either.
    TEST_ASSERT_EQUAL(BleStatus::NotFound,
                      client.setSlotNickname(0, ChameleonSense::None, "x", 1000));
    TEST_ASSERT_EQUAL(0, fake.writeCalls);
}

void test_an_active_slot_byte_outside_the_eight_is_not_a_slot() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1018, {0x09}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    uint8_t active = 0xFF;
    TEST_ASSERT_EQUAL(BleStatus::IoError, client.readActiveSlot(active, 1000));
    TEST_ASSERT_EQUAL_HEX8(0xFF, active);  // left alone, not overwritten
}

void test_setting_the_active_slot_sends_one_byte() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1003, {}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.setActiveSlot(6, 1000));
    ChameleonFrame frame;
    const std::vector<uint8_t>& sent = fake.lastWrite().data;
    TEST_ASSERT_TRUE(decodeChameleonFrame(sent.data(), sent.size(), frame) > 0);
    TEST_ASSERT_EQUAL_UINT16(1, frame.length);
    TEST_ASSERT_EQUAL_HEX8(0x06, frame.data[0]);
}

void test_saving_settings_is_its_own_command() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    fake.queueReply(rx(), tx(), replyFrame(1013, {}));

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.saveSettings(1000));
    ChameleonFrame frame;
    const std::vector<uint8_t>& sent = fake.lastWrite().data;
    TEST_ASSERT_TRUE(decodeChameleonFrame(sent.data(), sent.size(), frame) > 0);
    TEST_ASSERT_EQUAL_HEX16(1013, frame.command);
    // No payload: SAVE_SETTINGS takes none, and sending one would be refused.
    TEST_ASSERT_EQUAL_UINT16(0, frame.length);
}

// ─── Teardown ──────────────────────────────────────────────────────────────

void test_end_releases_the_subscription_and_the_controller() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);

    ChameleonUltraClient client(fake);
    client.begin();
    client.connect(FakeBleCentral::parseAddress(kUnitAddress), 1000);
    TEST_ASSERT_TRUE(fake.connected());
    TEST_ASSERT_EQUAL_UINT8(1, fake.subscriptionCount());

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());

    // Read off the transport, so a teardown that only claimed to happen cannot
    // pass: a subscription left on a dead handle is the leak FeatureSwitchLedger
    // models as the white screen.
    TEST_ASSERT_FALSE(fake.connected());
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
    TEST_ASSERT_FALSE(client.linkReady());
    TEST_ASSERT_EQUAL_UINT8(0, client.unitCount());
}

void test_end_stops_a_scan_left_running() {
    FakeBleCentral fake;
    scriptConnectedUltra(fake);
    ChameleonUltraClient client(fake);
    client.begin();

    // A scan in flight when the feature is left.
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.startScan(0, true));
    TEST_ASSERT_TRUE(fake.scanning());
    client.end();
    TEST_ASSERT_FALSE(fake.scanning());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_the_command_link_is_the_128_bit_nordic_uart_service);
    RUN_TEST(test_a_scan_keeps_only_connectable_peers_that_advertise_the_uart_service);
    RUN_TEST(test_connecting_exposes_the_command_link);
    RUN_TEST(test_a_peer_without_the_uart_service_is_not_a_connection);
    RUN_TEST(test_auto_connect_adopts_the_first_unit_and_records_it);
    RUN_TEST(test_auto_connect_does_not_steal_a_unit_already_held);
    RUN_TEST(test_auto_connect_with_no_units_is_not_found);
    RUN_TEST(test_a_command_returns_its_own_reply);
    RUN_TEST(test_a_reply_for_a_different_command_is_not_accepted);
    RUN_TEST(test_no_reply_at_all_is_a_timeout);
    RUN_TEST(test_a_command_before_the_link_is_up_is_disconnected);
    RUN_TEST(test_a_reply_longer_than_the_mtu_is_reassembled_from_notifications);
    RUN_TEST(test_a_reply_of_the_wrong_size_is_not_used_as_the_answer);
    RUN_TEST(test_a_failed_status_is_an_answer_and_not_a_link_error);
    RUN_TEST(test_the_device_address_is_a_static_random_address_not_an_untyped_number);
    RUN_TEST(test_a_device_address_of_the_wrong_length_is_refused);
    RUN_TEST(test_the_app_version_reads_major_then_minor);
    RUN_TEST(test_the_slot_kinds_come_from_the_firmware_tag_numbers);
    RUN_TEST(test_a_tag_this_build_cannot_name_is_unrecognised_not_guessed);
    RUN_TEST(test_an_old_style_tag_number_is_migrated_before_it_is_mapped);
    RUN_TEST(test_a_slot_table_of_old_numbers_still_names_cards);
    RUN_TEST(test_a_nickname_goes_out_as_slot_sense_and_name);
    RUN_TEST(test_a_nickname_that_does_not_fit_the_field_is_refused);
    RUN_TEST(test_a_slot_with_no_nickname_is_not_found_not_an_empty_string);
    RUN_TEST(test_a_recorded_nickname_comes_back_verbatim);
    RUN_TEST(test_a_slot_index_outside_the_eight_is_refused_before_anything_is_sent);
    RUN_TEST(test_an_active_slot_byte_outside_the_eight_is_not_a_slot);
    RUN_TEST(test_setting_the_active_slot_sends_one_byte);
    RUN_TEST(test_saving_settings_is_its_own_command);
    RUN_TEST(test_end_releases_the_subscription_and_the_controller);
    RUN_TEST(test_end_stops_a_scan_left_running);
    return UNITY_END();
}
