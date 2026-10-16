// SkeletonKey over a scripted BLE controller.
//
// The model suite (test_skeleton_key) covers the state machine and the tree
// accounting on their own. This one covers the seam between them and a radio:
// what a scan result becomes, what a discovery becomes, and what a read does
// when the peer's value does not fit the first buffer. Those conversions are
// where a client goes quietly wrong -- a UUID collapsed to 16 bits, a partial
// tree rendered as complete, a short read reported as the value -- and each is
// asserted here rather than trusted.

#include <unity.h>

#include <cstring>
#include <string>

#include "FakeBleCentral.h"
#include "features/ble/SkeletonKeyClient.h"

namespace {

BleAddress addressOf(const char* text) {
    return FakeBleCentral::parseAddress(text);
}

// A peer that looks like an RGB LED controller: the vendor service SkeletonKey
// has a one-tap profile for, plus a standard Battery service so a name lookup
// has something to resolve.
void scriptRgbController(FakeBleCentral& fake) {
    fake.connect(addressOf("AA:00:00:00:00:01"), 1000);
    BleDiscoveredService* vendor = fake.addService(0xFFF0);
    TEST_ASSERT_NOT_NULL(vendor);
    fake.addCharacteristic(0, 0xFFF3, kBlePropWrite | kBlePropWriteNoResponse);
    fake.addCharacteristic(0, 0xFFF4, kBlePropRead | kBlePropNotify);
    BleDiscoveredService* battery = fake.addService(0x180F);
    TEST_ASSERT_NOT_NULL(battery);
    fake.addCharacteristic(1, 0x2A19, kBlePropRead | kBlePropNotify);
}

void countNotify(void* context, const BleCharacteristicRef& ref, const uint8_t* data,
                 uint16_t length) {
    (void)ref;
    (void)data;
    (void)length;
    if (context != nullptr) {
        ++(*static_cast<int*>(context));
    }
}

}  // namespace

// ─── Scan ──────────────────────────────────────────────────────────────────

void test_a_scan_offers_only_connectable_peers_and_names_them() {
    FakeBleCentral fake;
    fake.addAdvertisement("AA:00:00:00:00:01", -40, "LED-1");
    // A non-connectable beacon has nothing a GATT explorer can open, so it must
    // not appear in the list at all.
    fake.addAdvertisement("AA:00:00:00:00:02", -30, "beacon", false);
    // No name at all: the device stays unnamed rather than being labelled.
    fake.addAdvertisement("AA:00:00:00:00:03", -70, nullptr);

    SkeletonKeyClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_TRUE(client.beginSession(true));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(true));

    TEST_ASSERT_EQUAL_UINT16(2, client.deviceCount());
    TEST_ASSERT_EQUAL_STRING("AA:00:00:00:00:01", client.devices()[0].address.c_str());
    TEST_ASSERT_EQUAL_STRING("LED-1", client.devices()[0].name.c_str());
    TEST_ASSERT_EQUAL_INT8(-40, client.devices()[0].rssi);
    TEST_ASSERT_EQUAL_STRING("", client.devices()[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("AA:00:00:00:00:03", client.devices()[1].address.c_str());

    // The scan was stopped again, so the controller is not left scanning.
    TEST_ASSERT_FALSE(fake.scanning());
}

void test_a_repeated_peer_updates_its_row_instead_of_appearing_twice() {
    FakeBleCentral fake;
    fake.addAdvertisement("AA:00:00:00:00:01", -40, "LED-1");
    fake.addAdvertisement("AA:00:00:00:00:01", -25, "LED-1");

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(true));

    TEST_ASSERT_EQUAL_UINT16(1, client.deviceCount());
    // The stronger of the two sightings is kept.
    TEST_ASSERT_EQUAL_INT8(-25, client.devices()[0].rssi);
}

void test_the_scan_list_is_capped_rather_than_grown_without_bound() {
    FakeBleCentral fake;
    char text[18] = {};
    for (uint16_t index = 0; index < SkeletonKeyClient::kBleMaxScanList + 8; ++index) {
        std::snprintf(text, sizeof(text), "AA:00:00:00:%02X:%02X",
                      static_cast<unsigned>(index >> 8), static_cast<unsigned>(index & 0xFF));
        fake.addAdvertisement(text, -50, "peer");
    }

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(true));
    TEST_ASSERT_EQUAL_UINT16(SkeletonKeyClient::kBleMaxScanList, client.deviceCount());
}

void test_an_address_keeps_its_type_on_the_way_into_the_row() {
    FakeBleCentral fake;
    BleAdvertisement advertisement;
    advertisement.address = addressOf("11:22:33:44:55:66");
    advertisement.address.type = BleAddressType::Random;
    advertisement.rssi = -50;
    advertisement.connectable = true;
    fake.addAdvertisement(advertisement);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(true));

    // The model keeps the peer as printable text, which drops the address type.
    // The client must retain the bytes alongside it -- re-parsing the printed
    // form would silently downgrade a random address to Unknown.
    BleAddress recovered;
    TEST_ASSERT_TRUE(client.addressAt(0, recovered));
    TEST_ASSERT_TRUE(recovered.type == BleAddressType::Random);
    TEST_ASSERT_EQUAL_STRING("11:22:33:44:55:66", client.devices()[0].address.c_str());
    TEST_ASSERT_FALSE(client.addressAt(1, recovered));
}

void test_a_second_scan_while_one_is_live_is_busy() {
    FakeBleCentral fake;
    SkeletonKeyClient client(fake);
    client.beginSession(true);

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.beginScan(false));
    TEST_ASSERT_EQUAL(BleStatus::Busy, client.beginScan(false));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.endScan());
}

// ─── Connect ───────────────────────────────────────────────────────────────

void test_connecting_to_the_strongest_peer_picks_the_right_one() {
    FakeBleCentral fake;
    fake.addAdvertisement("AA:00:00:00:00:01", -70, "weak");
    fake.addAdvertisement("AA:00:00:00:00:02", -35, "strong");
    fake.addAdvertisement("AA:00:00:00:00:03", -55, "middling");

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(false));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.connectToStrongest(1000));
    TEST_ASSERT_TRUE(client.connected());
    TEST_ASSERT_TRUE(fake.connectedAddress() == addressOf("AA:00:00:00:00:02"));
}

void test_connecting_with_an_empty_scan_list_is_not_found() {
    FakeBleCentral fake;
    SkeletonKeyClient client(fake);
    client.beginSession(true);

    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.connectToStrongest(1000));
    TEST_ASSERT_FALSE(client.connected());
    // Nothing was attempted, so the controller was never asked to connect.
    TEST_ASSERT_EQUAL(0, fake.connectCalls);
}

void test_a_failed_connection_leaves_the_session_failed_not_ready() {
    FakeBleCentral fake;
    fake.connectResult = BleStatus::ConnectFailed;

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::ConnectFailed,
                      client.connect(addressOf("AA:00:00:00:00:01"), 1000));
    TEST_ASSERT_TRUE(client.session().state() == SkeletonState::Failed);
    TEST_ASSERT_FALSE(client.canWriteFullDump());
}

// ─── Discovery → tree ──────────────────────────────────────────────────────

void test_discovery_builds_a_named_tree_with_full_uuids() {
    FakeBleCentral fake;
    scriptRgbController(fake);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    TEST_ASSERT_EQUAL(2, static_cast<int>(tree.services.size()));
    // Full 128-bit text, not the 16-bit field: a dump has to be unambiguous.
    TEST_ASSERT_EQUAL_STRING("0000FFF0-0000-1000-8000-00805F9B34FB",
                             tree.services[0].uuid.c_str());
    // Names resolved here, so the screen and the SD dump agree.
    TEST_ASSERT_EQUAL_STRING("Triones / ELK-BLEDOM LED service (vendor)",
                             tree.services[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Battery", tree.services[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Battery Level",
                             tree.services[1].characteristics[0].name.c_str());

    TEST_ASSERT_EQUAL(3, static_cast<int>(tree.characteristicCount()));
    TEST_ASSERT_EQUAL(2, static_cast<int>(tree.readableCount()));
    TEST_ASSERT_EQUAL(1, static_cast<int>(tree.writableCount()));
    TEST_ASSERT_FALSE(tree.truncated);
    TEST_ASSERT_TRUE(client.canWriteFullDump());
    TEST_ASSERT_EQUAL(1, fake.discoverCalls);
}

void test_a_truncated_discovery_is_refused_as_a_full_dump() {
    FakeBleCentral fake;
    scriptRgbController(fake);
    // The peer offered more characteristics than the fixed array holds. The tree
    // that comes out is partial, and a partial tree must not be written to the
    // card under the name "full dump".
    fake.peer().services[0].truncated = true;
    fake.peer().truncated = true;

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    TEST_ASSERT_TRUE(tree.truncated);
    TEST_ASSERT_FALSE(client.canWriteFullDump());
}

void test_a_discovery_with_no_services_is_a_failure_not_an_empty_tree() {
    FakeBleCentral fake;
    fake.connect(addressOf("AA:00:00:00:00:01"), 1000);
    // Nothing added: the peer answers discovery with an empty tree.

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::NoService, client.discover(tree));
    TEST_ASSERT_TRUE(client.session().state() == SkeletonState::Failed);
    TEST_ASSERT_FALSE(client.canWriteFullDump());
    // And the caller's tree was left alone: a half-built tree must not be
    // rendered as if it were the peer.
    TEST_ASSERT_EQUAL(0, static_cast<int>(tree.services.size()));
}

void test_discovery_without_a_connection_is_disconnected() {
    FakeBleCentral fake;
    SkeletonKeyClient client(fake);
    client.beginSession(true);

    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Disconnected, client.discover(tree));
    TEST_ASSERT_TRUE(client.session().state() == SkeletonState::Failed);
    TEST_ASSERT_EQUAL(0, fake.discoverCalls);
}

// ─── Read / write against a tree entry ─────────────────────────────────────

void test_a_value_longer_than_the_mtu_is_retried_rather_than_short_read() {
    FakeBleCentral fake;
    scriptRgbController(fake);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    // 24 bytes on a default 20-byte payload: the first read is refused with the
    // size needed, and the client must come back for the whole value.
    uint8_t payload[24] = {};
    for (uint8_t index = 0; index < 24; ++index) {
        payload[index] = static_cast<uint8_t>(index + 1);
    }
    fake.queueReadOk(payload, sizeof(payload));
    fake.queueReadOk(payload, sizeof(payload));

    TEST_ASSERT_EQUAL(BleStatus::Ok,
                      client.readIntoTree(tree, 0, 1, 1000));
    const std::string& value = tree.services[0].characteristics[1].lastValue;
    TEST_ASSERT_EQUAL(48, static_cast<int>(value.size()));
    TEST_ASSERT_EQUAL_STRING("0102030405060708090A0B0C0D0E0F101112131415161718", value.c_str());
    TEST_ASSERT_EQUAL(2, fake.readCalls);
}

void test_reading_a_missing_tree_entry_is_refused() {
    FakeBleCentral fake;
    scriptRgbController(fake);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.readIntoTree(tree, 9, 0, 1000));
    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.readIntoTree(tree, 0, 9, 1000));
    // Nothing was sent, so a bad index cannot silently read a neighbouring slot.
    TEST_ASSERT_EQUAL(0, fake.readCalls);
}

void test_a_write_goes_to_the_characteristic_the_tree_names() {
    FakeBleCentral fake;
    scriptRgbController(fake);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    const uint8_t colour[3] = {0xFF, 0x00, 0x80};
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.writeToTree(tree, 0, 0, colour, 3, false));
    TEST_ASSERT_EQUAL(1, static_cast<int>(fake.writeCount()));
    TEST_ASSERT_EQUAL_STRING(
        "0000FFF0-0000-1000-8000-00805F9B34FB",
        SkeletonKeyClient::uuidText(fake.lastWrite().ref.service).c_str());
    TEST_ASSERT_EQUAL_STRING(
        "0000FFF3-0000-1000-8000-00805F9B34FB",
        SkeletonKeyClient::uuidText(fake.lastWrite().ref.characteristic).c_str());
    TEST_ASSERT_EQUAL(3, static_cast<int>(fake.lastWrite().data.size()));
    TEST_ASSERT_FALSE(fake.lastWrite().withResponse);
}

// ─── Notify, and the teardown ──────────────────────────────────────────────

void test_a_subscription_can_be_taken_and_released() {
    FakeBleCentral fake;
    scriptRgbController(fake);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF4);
    int notifications = 0;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.subscribe(ref, countNotify, &notifications));
    TEST_ASSERT_EQUAL_UINT8(1, fake.subscriptionCount());

    const uint8_t update[2] = {0x7F, 0x00};
    TEST_ASSERT_EQUAL_UINT8(1, fake.deliver(ref, update, sizeof(update)));
    TEST_ASSERT_EQUAL(1, notifications);

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.unsubscribe(ref));
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());
}

void test_end_releases_the_controller_and_the_peer_list() {
    FakeBleCentral fake;
    fake.addAdvertisement("AA:00:00:00:00:01", -40, "LED-1");
    scriptRgbController(fake);

    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(false));
    TEST_ASSERT_EQUAL_UINT16(1, client.deviceCount());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.connectToStrongest(1000));

    const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0xFFF0, 0xFFF4);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.subscribe(ref, countNotify, nullptr));
    SkeletonTree tree;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.discover(tree));

    TEST_ASSERT_TRUE(fake.connected());
    TEST_ASSERT_EQUAL_UINT8(1, fake.subscriptionCount());

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());

    // Read off the transport, not the client's own flags: a teardown that only
    // claims to have happened is the white-screen bug itself.
    TEST_ASSERT_FALSE(fake.connected());
    TEST_ASSERT_FALSE(fake.scanning());
    TEST_ASSERT_EQUAL_UINT8(0, fake.subscriptionCount());
    TEST_ASSERT_EQUAL(1, fake.endCalls);

    // And the session's own state is back to what "no session" looks like.
    TEST_ASSERT_TRUE(client.session().state() == SkeletonState::Idle);
    TEST_ASSERT_EQUAL_UINT16(0, client.deviceCount());
    TEST_ASSERT_FALSE(client.canWriteFullDump());
}

void test_end_stops_a_scan_that_is_still_running() {
    FakeBleCentral fake;
    SkeletonKeyClient client(fake);
    client.beginSession(true);
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.beginScan(true));
    TEST_ASSERT_TRUE(fake.scanning());

    client.end();
    // A scan left running is the second white-screen cause; end() must close it
    // rather than only dropping the connection.
    TEST_ASSERT_FALSE(fake.scanning());
    TEST_ASSERT_EQUAL(1, fake.stopScanCalls);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_a_scan_offers_only_connectable_peers_and_names_them);
    RUN_TEST(test_a_repeated_peer_updates_its_row_instead_of_appearing_twice);
    RUN_TEST(test_the_scan_list_is_capped_rather_than_grown_without_bound);
    RUN_TEST(test_an_address_keeps_its_type_on_the_way_into_the_row);
    RUN_TEST(test_a_second_scan_while_one_is_live_is_busy);
    RUN_TEST(test_connecting_to_the_strongest_peer_picks_the_right_one);
    RUN_TEST(test_connecting_with_an_empty_scan_list_is_not_found);
    RUN_TEST(test_a_failed_connection_leaves_the_session_failed_not_ready);
    RUN_TEST(test_discovery_builds_a_named_tree_with_full_uuids);
    RUN_TEST(test_a_truncated_discovery_is_refused_as_a_full_dump);
    RUN_TEST(test_a_discovery_with_no_services_is_a_failure_not_an_empty_tree);
    RUN_TEST(test_discovery_without_a_connection_is_disconnected);
    RUN_TEST(test_a_value_longer_than_the_mtu_is_retried_rather_than_short_read);
    RUN_TEST(test_reading_a_missing_tree_entry_is_refused);
    RUN_TEST(test_a_write_goes_to_the_characteristic_the_tree_names);
    RUN_TEST(test_a_subscription_can_be_taken_and_released);
    RUN_TEST(test_end_releases_the_controller_and_the_peer_list);
    RUN_TEST(test_end_stops_a_scan_that_is_still_running);
    return UNITY_END();
}
