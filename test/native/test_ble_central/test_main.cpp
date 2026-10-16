// The BLE central seam, and the contract the scripted fake promises.
//
// Two reasons this suite exists before any client suite does:
//
//   1. The shared primitives are protocol code in their own right. UUID and
//      address handling is where a client silently talks to the wrong peer, and
//      the status labels are what the honest-report line prints.
//   2. The three client suites (SkeletonKey, Chameleon Ultra, HID reader) read
//      their expectations off the fake. If the fake is wrong, they all agree on
//      something false -- so its contract is pinned here rather than assumed
//      there.
//
// The resource accounting assertions matter most: `end()` clearing the
// connection and the subscriptions is the mechanism FeatureSwitchLedger's
// white-screen model depends on, and a teardown that only *claims* to happen is
// exactly the bug that model exists for.

#include <unity.h>

#include <cstring>

#include "drivers/IBleCentral.h"
#include "FakeBleCentral.h"

// ─── Status labels ─────────────────────────────────────────────────────────

void test_every_status_has_a_label_of_its_own() {
    const BleStatus all[] = {
        BleStatus::Ok,       BleStatus::NotFound,     BleStatus::NoService,
        BleStatus::NotPermitted, BleStatus::TooLong,  BleStatus::ConnectFailed,
        BleStatus::Disconnected, BleStatus::Timeout,  BleStatus::Busy,
        BleStatus::IoError,  BleStatus::Unsupported,
    };
    const int count = static_cast<int>(sizeof(all) / sizeof(all[0]));
    for (int i = 0; i < count; ++i) {
        const char* label = bleStatusLabel(all[i]);
        TEST_ASSERT_NOT_NULL(label);
        TEST_ASSERT_GREATER_THAN(0, static_cast<int>(std::strlen(label)));
        // "unknown" is the fallthrough, so seeing it means an enumerator was
        // added without a label.
        TEST_ASSERT_NOT_EQUAL(0, std::strcmp("unknown", label));
    }

    // The two that must never be confused with each other.
    TEST_ASSERT_EQUAL_STRING("not permitted", bleStatusLabel(BleStatus::NotPermitted));
    TEST_ASSERT_EQUAL_STRING("io error", bleStatusLabel(BleStatus::IoError));
}

// ─── UUIDs ─────────────────────────────────────────────────────────────────

void test_a_sig_short_uuid_expands_to_its_base_form() {
    const BleUuid battery = BleUuid::fromShort(0x180F);
    char text[37] = {};
    TEST_ASSERT_TRUE(battery.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("0000180F-0000-1000-8000-00805F9B34FB", text);

    TEST_ASSERT_TRUE(battery.valid());
    TEST_ASSERT_TRUE(battery.isShort());
    TEST_ASSERT_EQUAL_HEX16(0x180F, battery.shortValue());
}

void test_a_128_bit_vendor_uuid_is_not_collapsed_to_its_offset() {
    // The Chameleon Ultra's command link is Nordic's UART service. Nordic's own
    // documentation calls it "16-bit offset 0x0001", and that phrasing is a
    // trap: 0x0001 is *not* this service. This is the assertion that keeps a
    // client from addressing the wrong characteristic with a confident-looking
    // short UUID.
    const uint8_t nus[16] = {0x6E, 0x40, 0x00, 0x01, 0xB5, 0xA3, 0xF3, 0x93,
                             0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E};
    const BleUuid service = BleUuid::fromBytes(nus);

    char text[37] = {};
    TEST_ASSERT_TRUE(service.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("6E400001-B5A3-F393-E0A9-E50E24DCCA9E", text);

    TEST_ASSERT_TRUE(service.valid());
    TEST_ASSERT_FALSE(service.isShort());
    // 0 rather than 0x0001: a caller that skips isShort() must not be handed a
    // plausible-looking wrong answer.
    TEST_ASSERT_EQUAL_HEX16(0x0000, service.shortValue());

    // And a short UUID must never equal the 128-bit one it superficially
    // resembles.
    TEST_ASSERT_TRUE(service != BleUuid::fromShort(0x0001));
    TEST_ASSERT_TRUE(BleUuid::fromShort(0x0001).isShort());
}

void test_an_unset_uuid_is_not_valid_and_a_short_buffer_refuses() {
    BleUuid unset;
    TEST_ASSERT_FALSE(unset.valid());
    TEST_ASSERT_FALSE(unset.isShort());

    const BleUuid known = BleUuid::fromShort(0x180F);
    char tooSmall[36] = {};
    TEST_ASSERT_FALSE(known.write(tooSmall, sizeof(tooSmall)));
    TEST_ASSERT_EQUAL_CHAR('\0', tooSmall[0]);
    TEST_ASSERT_FALSE(known.write(nullptr, 37));
    TEST_ASSERT_FALSE(known.write(tooSmall, 0));
}

void test_an_advertisement_reports_the_services_it_lists() {
    BleAdvertisement advertisement;
    advertisement.serviceCount = 2;
    advertisement.serviceUuids[0] = BleUuid::fromShort(0xFFF0);
    advertisement.serviceUuids[1] = BleUuid::fromShort(0x180F);

    TEST_ASSERT_TRUE(advertisement.advertises(BleUuid::fromShort(0x180F)));
    TEST_ASSERT_TRUE(advertisement.advertises(BleUuid::fromShort(0xFFF0)));
    TEST_ASSERT_FALSE(advertisement.advertises(BleUuid::fromShort(0x1800)));
}

// ─── Address handling ──────────────────────────────────────────────────────

void test_an_unset_address_is_not_valid() {
    BleAddress address;
    TEST_ASSERT_FALSE(address.valid());

    // One non-zero byte is enough: no real peer is all zeros.
    address.bytes[5] = 0x01;
    TEST_ASSERT_TRUE(address.valid());
}

void test_an_address_prints_in_the_order_every_tool_uses() {
    BleAddress address;
    const uint8_t bytes[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    std::memcpy(address.bytes.data(), bytes, 6);

    char text[18] = {};
    TEST_ASSERT_TRUE(address.write(text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:11:22:33", text);
    TEST_ASSERT_EQUAL(17, static_cast<int>(std::strlen(text)));

    // A short buffer must refuse, not truncate into a plausible wrong address.
    char tooSmall[17] = {};
    TEST_ASSERT_FALSE(address.write(tooSmall, sizeof(tooSmall)));
    TEST_ASSERT_EQUAL_CHAR('\0', tooSmall[0]);
    TEST_ASSERT_FALSE(address.write(nullptr, 18));
    TEST_ASSERT_FALSE(address.write(text, 0));
}

void test_the_address_type_is_part_of_the_identity() {
    BleAddress publicOne;
    BleAddress randomOne;
    const uint8_t bytes[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    std::memcpy(publicOne.bytes.data(), bytes, 6);
    std::memcpy(randomOne.bytes.data(), bytes, 6);
    publicOne.type = BleAddressType::Public;
    randomOne.type = BleAddressType::Random;

    // Same 48 bits, different type: two different peers. Connecting to the wrong
    // one is a real failure mode, so equality must not collapse them.
    TEST_ASSERT_FALSE(publicOne == randomOne);
    TEST_ASSERT_TRUE(publicOne != randomOne);

    BleAddress sameAsPublic = publicOne;
    TEST_ASSERT_TRUE(publicOne == sameAsPublic);

    // The printed form carries the 48 bits but not the address type, so a
    // round-trip through text must not be treated as reconstructing the peer.
    char text[18] = {};
    TEST_ASSERT_TRUE(publicOne.write(text, sizeof(text)));
    const BleAddress reparsed = FakeBleCentral::parseAddress(text);
    TEST_ASSERT_TRUE(reparsed.type == BleAddressType::Unknown);
    TEST_ASSERT_EQUAL(0, std::memcmp(publicOne.bytes.data(), reparsed.bytes.data(), 6));
    TEST_ASSERT_TRUE(publicOne != reparsed);
}

void test_an_unset_characteristic_reference_is_rejected() {
    BleCharacteristicRef ref;
    TEST_ASSERT_FALSE(ref.valid());

    ref.service = BleUuid::fromShort(0xFFF0);
    TEST_ASSERT_FALSE(ref.valid());  // still no characteristic
    ref.characteristic = BleUuid::fromShort(0xFFF3);
    TEST_ASSERT_TRUE(ref.valid());

    // Same characteristic UUID under two services is two different values.
    TEST_ASSERT_TRUE(ref != BleCharacteristicRef::fromShort(0xFFF1, 0xFFF3));
    TEST_ASSERT_TRUE(ref == BleCharacteristicRef::fromShort(0xFFF0, 0xFFF3));
}

// ─── The fake's contract ───────────────────────────────────────────────────

void test_scan_results_are_capped_and_counted() {
    FakeBleCentral fake;
    fake.addAdvertisement("AA:00:00:00:00:01", -40, "one");
    fake.addAdvertisement("AA:00:00:00:00:02", -50, "two");
    fake.addAdvertisement("AA:00:00:00:00:03", -60, "three");

    TEST_ASSERT_FALSE(fake.scanning());
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.startScan(0, true));
    TEST_ASSERT_TRUE(fake.scanning());
    // A second scan on top of a live one is Busy, not a silent restart.
    TEST_ASSERT_EQUAL(BleStatus::Busy, fake.startScan(0, true));

    BleAdvertisement results[8];
    TEST_ASSERT_EQUAL_UINT8(3, fake.scanResults(results, 8));
    TEST_ASSERT_EQUAL_STRING("one", results[0].name);
    TEST_ASSERT_EQUAL_INT8(-40, results[0].rssi);
    TEST_ASSERT_TRUE(results[0].connectable);

    // The result count is capped by the caller's capacity, not the scan list.
    TEST_ASSERT_EQUAL_UINT8(2, fake.scanResults(results, 2));
    TEST_ASSERT_EQUAL_UINT8(0, fake.scanResults(results, 0));
    TEST_ASSERT_EQUAL_UINT8(0, fake.scanResults(nullptr, 4));

    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.stopScan());
    TEST_ASSERT_FALSE(fake.scanning());
}

void test_connecting_to_an_unset_address_is_not_found() {
    FakeBleCentral fake;

    TEST_ASSERT_EQUAL(BleStatus::NotFound, fake.connect(BleAddress{}, 1000));
    TEST_ASSERT_FALSE(fake.connected());
    TEST_ASSERT_EQUAL(1, fake.connectCalls);

    // A refused connect must not leave a half-open connection behind.
    fake.connectResult = BleStatus::ConnectFailed;
    const BleAddress peer = FakeBleCentral::parseAddress("AA:00:00:00:00:01");
    TEST_ASSERT_EQUAL(BleStatus::ConnectFailed, fake.connect(peer, 1000));
    TEST_ASSERT_FALSE(fake.connected());

    fake.connectResult = BleStatus::Ok;
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.connect(peer, 1000));
    TEST_ASSERT_TRUE(fake.connected());
    TEST_ASSERT_TRUE(fake.connectedAddress() == peer);
}

void test_discovery_requires_a_connection() {
    FakeBleCentral fake;
    BleDiscoveredPeer peer;

    TEST_ASSERT_EQUAL(BleStatus::Disconnected, fake.discover(peer));
    TEST_ASSERT_EQUAL_UINT8(0, peer.serviceCount);

    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    TEST_ASSERT_NOT_NULL(fake.addService(0xFFF0));
    TEST_ASSERT_NOT_NULL(
        fake.addCharacteristic(0, 0xFFF3, kBlePropWrite | kBlePropWriteNoResponse));

    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.discover(peer));
    TEST_ASSERT_EQUAL_UINT8(1, peer.serviceCount);
    TEST_ASSERT_TRUE(peer.services[0].uuid == BleUuid::fromShort(0xFFF0));
    TEST_ASSERT_EQUAL_UINT8(1, peer.services[0].characteristicCount);
    TEST_ASSERT_TRUE(peer.services[0].characteristics[0].uuid == BleUuid::fromShort(0xFFF3));
    TEST_ASSERT_EQUAL_HEX8(kBlePropWrite | kBlePropWriteNoResponse,
                           peer.services[0].characteristics[0].properties);
    TEST_ASSERT_FALSE(peer.truncated);

    // A service index that was never added must miss rather than write into a
    // neighbouring slot.
    TEST_ASSERT_NULL(fake.addCharacteristic(3, 0x2A19, kBlePropRead));
}

void test_overfilling_the_fixed_arrays_is_marked_not_dropped() {
    FakeBleCentral fake;
    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);

    for (uint8_t index = 0; index < kBleMaxServicesPerPeer; ++index) {
        TEST_ASSERT_NOT_NULL(fake.addService(static_cast<uint16_t>(0x1800 + index)));
    }
    // One past the end: refused, and the overflow is recorded so a tree model
    // can say "truncated" instead of presenting a partial tree as complete.
    TEST_ASSERT_NULL(fake.addService(0x2A00));
    TEST_ASSERT_TRUE(fake.peer().truncated);
    TEST_ASSERT_EQUAL_UINT8(kBleMaxServicesPerPeer, fake.peer().serviceCount);

    // Same for the per-service characteristic array.
    for (uint8_t index = 0; index < kBleMaxCharsPerService; ++index) {
        TEST_ASSERT_NOT_NULL(fake.addCharacteristic(0, static_cast<uint16_t>(0x2A00 + index),
                                                    kBlePropRead));
    }
    TEST_ASSERT_NULL(fake.addCharacteristic(0, 0x2B00, kBlePropRead));
    TEST_ASSERT_TRUE(fake.peer().services[0].truncated);
}

void test_an_unscripted_read_fails_loudly_instead_of_returning_zero_bytes() {
    FakeBleCentral fake;
    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF3);

    uint8_t buffer[32] = {};
    uint16_t length = sizeof(buffer);
    TEST_ASSERT_EQUAL(BleStatus::IoError, fake.read(ref, buffer, length, 1000));
    TEST_ASSERT_EQUAL_UINT16(0, length);
    TEST_ASSERT_EQUAL(1, fake.unscriptedReads);
}

void test_a_reply_too_large_for_the_caller_buffer_reports_the_size_needed() {
    FakeBleCentral fake;
    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    const uint8_t payload[5] = {1, 2, 3, 4, 5};
    fake.queueReadOk(payload, sizeof(payload));

    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF3);
    uint8_t small[3] = {};
    uint16_t length = sizeof(small);
    TEST_ASSERT_EQUAL(BleStatus::TooLong, fake.read(ref, small, length, 1000));
    // The needed size, so the caller can retry -- never a silent short read.
    TEST_ASSERT_EQUAL_UINT16(5, length);

    // And a second call consumes the next scripted reply, not the same one again.
    fake.queueReadOk(payload, sizeof(payload));
    uint8_t big[8] = {};
    length = sizeof(big);
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.read(ref, big, length, 1000));
    TEST_ASSERT_EQUAL_UINT16(5, length);
}

void test_a_refused_write_is_still_recorded() {
    FakeBleCentral fake;
    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF3);
    const uint8_t payload[3] = {0x11, 0x22, 0x33};

    // Not connected: the write must fail *and* leave evidence that it was tried.
    // "Refused" and "never attempted" are different bugs.
    TEST_ASSERT_EQUAL(BleStatus::Disconnected, fake.write(ref, payload, 3, false));
    TEST_ASSERT_EQUAL(1, static_cast<int>(fake.writeCount()));
    TEST_ASSERT_EQUAL(3, static_cast<int>(fake.lastWrite().data.size()));
    TEST_ASSERT_FALSE(fake.lastWrite().withResponse);

    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    fake.writeResult = BleStatus::NotPermitted;
    TEST_ASSERT_EQUAL(BleStatus::NotPermitted, fake.write(ref, payload, 3, true));
    TEST_ASSERT_TRUE(fake.lastWrite().withResponse);
    TEST_ASSERT_EQUAL(2, static_cast<int>(fake.writeCount()));
}

void test_subscribing_requires_a_connection_and_counts() {
    FakeBleCentral fake;
    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF4);

    TEST_ASSERT_EQUAL(BleStatus::Disconnected, fake.subscribe(ref, nullptr, nullptr));
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());

    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.subscribe(ref, nullptr, nullptr));
    TEST_ASSERT_EQUAL_UINT8(1, fake.subscriptionCount());

    // Unsubscribing something that was never subscribed is not a success.
    const BleCharacteristicRef other = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF5);
    TEST_ASSERT_EQUAL(BleStatus::NotFound, fake.unsubscribe(other));
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.unsubscribe(ref));
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());
}

namespace {

void countNotification(void* context, const BleCharacteristicRef& ref, const uint8_t* data,
                       uint16_t length) {
    (void)ref;
    int* counter = static_cast<int*>(context);
    if (counter == nullptr) {
        return;
    }
    if (data == nullptr || length == 0) {
        // A notification with no payload must never be scored as a valid event;
        // -1 makes the test fail rather than pass quietly.
        *counter = -1;
        return;
    }
    ++(*counter);
}

}  // namespace

void test_notifications_reach_listeners_and_a_dead_letter_is_counted() {
    FakeBleCentral fake;
    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF4);

    int delivered = 0;
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.subscribe(ref, countNotification, &delivered));

    const uint8_t payload[2] = {0x7F, 0x00};
    TEST_ASSERT_EQUAL_UINT8(1, fake.deliver(ref, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(1, delivered);

    // Nothing listening: counted, not silently dropped, so a session that
    // expects a reply can tell that it never arrived.
    TEST_ASSERT_EQUAL(BleStatus::Ok, fake.unsubscribe(ref));
    TEST_ASSERT_EQUAL_UINT8(0, fake.deliver(ref, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(1, fake.undeliveredNotifications);
    TEST_ASSERT_EQUAL(1, delivered);
}

// ─── Teardown: the mechanism FeatureSwitchLedger relies on ─────────────────

void test_end_really_releases_the_controller() {
    FakeBleCentral fake;
    fake.connect(FakeBleCentral::parseAddress("AA:00:00:00:00:01"), 1000);
    fake.startScan(0, true);
    fake.subscribe(BleCharacteristicRef::fromShort(0xFFF0, 0xFFF4), countNotification,
                   nullptr);

    TEST_ASSERT_TRUE(fake.connected());
    TEST_ASSERT_EQUAL_UINT8(1, fake.subscriptionCount());
    TEST_ASSERT_TRUE(fake.scanning());

    fake.end();

    // A teardown that leaves the connection, the scan or a subscription alive is
    // the white-screen cause. Asserting it here means a client suite can read
    // "released" off the transport instead of trusting the session's own flag.
    TEST_ASSERT_FALSE(fake.connected());
    TEST_ASSERT_FALSE(fake.scanning());
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_status_has_a_label_of_its_own);
    RUN_TEST(test_a_sig_short_uuid_expands_to_its_base_form);
    RUN_TEST(test_a_128_bit_vendor_uuid_is_not_collapsed_to_its_offset);
    RUN_TEST(test_an_unset_uuid_is_not_valid_and_a_short_buffer_refuses);
    RUN_TEST(test_an_advertisement_reports_the_services_it_lists);
    RUN_TEST(test_an_unset_address_is_not_valid);
    RUN_TEST(test_an_address_prints_in_the_order_every_tool_uses);
    RUN_TEST(test_the_address_type_is_part_of_the_identity);
    RUN_TEST(test_an_unset_characteristic_reference_is_rejected);
    RUN_TEST(test_scan_results_are_capped_and_counted);
    RUN_TEST(test_connecting_to_an_unset_address_is_not_found);
    RUN_TEST(test_discovery_requires_a_connection);
    RUN_TEST(test_overfilling_the_fixed_arrays_is_marked_not_dropped);
    RUN_TEST(test_an_unscripted_read_fails_loudly_instead_of_returning_zero_bytes);
    RUN_TEST(test_a_reply_too_large_for_the_caller_buffer_reports_the_size_needed);
    RUN_TEST(test_a_refused_write_is_still_recorded);
    RUN_TEST(test_subscribing_requires_a_connection_and_counts);
    RUN_TEST(test_notifications_reach_listeners_and_a_dead_letter_is_counted);
    RUN_TEST(test_end_really_releases_the_controller);
    return UNITY_END();
}
