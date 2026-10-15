// SkeletonKey: the three things upstream v3.9.x added, minus the radio.
//
//   * the GATT tree  -- accounting plus name resolution (features/ble/GattNameTable.h)
//   * the full dump  -- written only from a tree that finished discovery
//   * the colour mixer -- integer maths, so device and host agree bit for bit

#include <unity.h>

#include <string>

#include "features/ble/SkeletonKeyModel.h"

namespace {

constexpr const char* kBatteryService = "0000180f-0000-1000-8000-00805f9b34fb";
constexpr const char* kBatteryLevel = "00002a19-0000-1000-8000-00805f9b34fb";
constexpr const char* kDeviceName = "00002a00-0000-1000-8000-00805f9b34fb";
constexpr const char* kLedService = "0000fff0-0000-1000-8000-00805f9b34fb";
constexpr const char* kLedControl = "0000fff3-0000-1000-8000-00805f9b34fb";
constexpr const char* kVendorUnknown = "12345678-1234-5678-1234-56789abcdef0";

SkeletonTree makeTree(bool withVendorService) {
    SkeletonTree tree;

    SkeletonService battery;
    battery.uuid = kBatteryService;
    SkeletonCharacteristic level;
    level.uuid = kBatteryLevel;
    level.properties =
        static_cast<uint8_t>(SkeletonKey::kPropertyRead | SkeletonKey::kPropertyNotify);
    SkeletonCharacteristic name;
    name.uuid = kDeviceName;
    name.properties = static_cast<uint8_t>(SkeletonKey::kPropertyRead | SkeletonKey::kPropertyWrite);
    battery.characteristics.push_back(level);
    battery.characteristics.push_back(name);
    tree.services.push_back(battery);

    if (withVendorService) {
        SkeletonService led;
        led.uuid = kLedService;
        SkeletonCharacteristic control;
        control.uuid = kLedControl;
        control.properties = SkeletonKey::kPropertyWriteWithoutResponse;
        led.characteristics.push_back(control);
        tree.services.push_back(led);
    }
    return tree;
}

}  // namespace

// ---------------------------------------------------------------------------
// GATT tree: accounting
// ---------------------------------------------------------------------------

void test_tree_counts_split_by_property() {
    const SkeletonTree tree = makeTree(true);
    TEST_ASSERT_EQUAL(2, tree.services.size());
    TEST_ASSERT_EQUAL(3, tree.characteristicCount());
    // Read (battery level, and Notify counts as readable too) + Read (device
    // name). The LED control is write-only.
    TEST_ASSERT_EQUAL(2, tree.readableCount());
    // Read+Write, and the LED control's Write-Without-Response.
    TEST_ASSERT_EQUAL(2, tree.writableCount());
    TEST_ASSERT_EQUAL(1, tree.notifyCount());

    const SkeletonTree empty;
    TEST_ASSERT_EQUAL(0, empty.characteristicCount());
    TEST_ASSERT_EQUAL(0, empty.readableCount());
    TEST_ASSERT_EQUAL(0, empty.writableCount());
    TEST_ASSERT_EQUAL(0, empty.notifyCount());
}

// ---------------------------------------------------------------------------
// GATT tree: names
// ---------------------------------------------------------------------------

void test_apply_names_fills_known_uuids_and_leaves_unknown_empty() {
    SkeletonTree tree = makeTree(true);
    SkeletonKey::applyNames(tree);

    TEST_ASSERT_EQUAL_STRING("Battery", tree.services[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Battery Level", tree.services[0].characteristics[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Device Name", tree.services[0].characteristics[1].name.c_str());
    // Vendor UUIDs on the SIG base resolve, and are marked as vendor.
    TEST_ASSERT_EQUAL_STRING("Triones / ELK-BLEDOM LED service (vendor)",
                             tree.services[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Triones / ELK-BLEDOM LED control (vendor)",
                             tree.services[1].characteristics[0].name.c_str());

    // A genuine 128-bit vendor UUID stays nameless: the caller prints the raw
    // UUID rather than a name we made up.
    SkeletonTree other;
    SkeletonService s;
    s.uuid = kVendorUnknown;
    SkeletonCharacteristic c;
    c.uuid = kVendorUnknown;
    s.characteristics.push_back(c);
    other.services.push_back(s);
    SkeletonKey::applyNames(other);
    TEST_ASSERT_EQUAL_STRING("", other.services[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("", other.services[0].characteristics[0].name.c_str());
}

void test_apply_names_defers_to_a_name_the_device_supplied() {
    SkeletonTree tree = makeTree(false);
    tree.services[0].name = "My Custom Battery";
    tree.services[0].characteristics[0].name = "Battery %";

    SkeletonKey::applyNames(tree);

    // A GATT User Description is authoritative; our table must not overrule it.
    TEST_ASSERT_EQUAL_STRING("My Custom Battery", tree.services[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Battery %", tree.services[0].characteristics[0].name.c_str());
    // The still-unnamed characteristic gets filled in.
    TEST_ASSERT_EQUAL_STRING("Device Name", tree.services[0].characteristics[1].name.c_str());

    // Re-running must not append or duplicate anything.
    SkeletonKey::applyNames(tree);
    TEST_ASSERT_EQUAL_STRING("My Custom Battery", tree.services[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Battery %", tree.services[0].characteristics[0].name.c_str());
}

// ---------------------------------------------------------------------------
// Session: the dump is only as good as the discovery that produced it
// ---------------------------------------------------------------------------

void test_begin_requires_confirmation_and_resets_prior_state() {
    SkeletonSession session;
    TEST_ASSERT_FALSE(session.begin(false));
    TEST_ASSERT_EQUAL_STRING("not authorised", session.lastError());
    TEST_ASSERT_EQUAL(SkeletonState::Idle, session.state());

    TEST_ASSERT_TRUE(session.begin(true));
    TEST_ASSERT_EQUAL(SkeletonState::Scanning, session.state());
    TEST_ASSERT_EQUAL(0, session.tree().services.size());
    TEST_ASSERT_FALSE(session.discoveryComplete());
}

void test_a_scan_that_finds_nothing_fails_rather_than_loitering() {
    SkeletonSession session;
    TEST_ASSERT_TRUE(session.begin(true));

    session.onScanComplete(0);
    TEST_ASSERT_EQUAL(SkeletonState::Failed, session.state());
    TEST_ASSERT_EQUAL_STRING("no devices found", session.lastError());
    TEST_ASSERT_EQUAL(0, session.deviceCount());

    TEST_ASSERT_TRUE(session.begin(true));
    session.onScanComplete(3);
    TEST_ASSERT_EQUAL(SkeletonState::Scanning, session.state());
    TEST_ASSERT_EQUAL(3, session.deviceCount());
    TEST_ASSERT_EQUAL_STRING("", session.lastError());
}

void test_connect_failure_is_reported_with_a_reason() {
    SkeletonSession session;
    TEST_ASSERT_TRUE(session.begin(true));
    session.onScanComplete(1);
    session.onConnectStarted();
    TEST_ASSERT_EQUAL(SkeletonState::Connecting, session.state());

    session.onConnectFailed("gatt 133");
    TEST_ASSERT_EQUAL(SkeletonState::Failed, session.state());
    TEST_ASSERT_EQUAL_STRING("gatt 133", session.lastError());

    // A null reason must not leave lastError() dangling.
    session.onConnectFailed(nullptr);
    TEST_ASSERT_EQUAL_STRING("connect failed", session.lastError());
}

void test_full_dump_is_refused_until_discovery_completes() {
    SkeletonSession session;
    TEST_ASSERT_TRUE(session.begin(true));
    session.onScanComplete(1);
    session.onConnectStarted();
    // Scanning and connected is still not enough: nothing has been discovered.
    TEST_ASSERT_FALSE(session.canWriteFullDump());

    // An empty discovery is a failure, and must not be dumpable -- otherwise a
    // zero-service "dump" would land on the card looking like a real result.
    session.onDiscoverComplete(SkeletonTree{});
    TEST_ASSERT_EQUAL(SkeletonState::Failed, session.state());
    TEST_ASSERT_EQUAL_STRING("no services discovered", session.lastError());
    TEST_ASSERT_FALSE(session.canWriteFullDump());

    session.onDiscoverComplete(makeTree(true));
    TEST_ASSERT_EQUAL(SkeletonState::Ready, session.state());
    TEST_ASSERT_TRUE(session.discoveryComplete());
    TEST_ASSERT_TRUE(session.canWriteFullDump());
    // Discovery is where names get resolved, so the dump and the screen agree.
    TEST_ASSERT_EQUAL_STRING("Battery", session.tree().services[0].name.c_str());

    session.onSendComplete();
    TEST_ASSERT_EQUAL(SkeletonState::Ready, session.state());
    TEST_ASSERT_TRUE(session.canWriteFullDump());
}

void test_stop_drops_the_tree_so_a_stale_dump_cannot_be_written() {
    SkeletonSession session;
    TEST_ASSERT_TRUE(session.begin(true));
    session.onScanComplete(1);
    session.onDiscoverComplete(makeTree(false));
    TEST_ASSERT_TRUE(session.canWriteFullDump());

    session.requestStop();
    TEST_ASSERT_EQUAL(SkeletonState::Idle, session.state());
    TEST_ASSERT_FALSE(session.canWriteFullDump());

    session.onDiscoverComplete(makeTree(false));
    session.stop();
    TEST_ASSERT_EQUAL(SkeletonState::Idle, session.state());
    TEST_ASSERT_EQUAL(0, session.tree().services.size());
    TEST_ASSERT_FALSE(session.discoveryComplete());
    TEST_ASSERT_FALSE(session.canWriteFullDump());
}

// ---------------------------------------------------------------------------
// Colour mix
// ---------------------------------------------------------------------------

void test_presets_and_payload_are_the_rgb_triples_the_controllers_expect() {
    TEST_ASSERT_EQUAL(255, SkeletonKey::presetWhite().red);
    TEST_ASSERT_EQUAL(255, SkeletonKey::presetWhite().green);
    TEST_ASSERT_EQUAL(255, SkeletonKey::presetWhite().blue);

    // Warm is red-dominant, cool is blue-dominant -- if these ever invert, the
    // sliders are mislabelled.
    TEST_ASSERT_TRUE(SkeletonKey::presetWarm().red > SkeletonKey::presetWarm().blue);
    TEST_ASSERT_TRUE(SkeletonKey::presetCool().blue > SkeletonKey::presetCool().red);

    SkeletonColor color;
    color.red = 0x11;
    color.green = 0x22;
    color.blue = 0x33;
    const std::vector<uint8_t> payload = SkeletonKey::colorPayload(color);
    TEST_ASSERT_EQUAL(3, payload.size());
    TEST_ASSERT_EQUAL(0x11, payload[0]);
    TEST_ASSERT_EQUAL(0x22, payload[1]);
    TEST_ASSERT_EQUAL(0x33, payload[2]);
}

void test_blend_endpoints_are_exact_and_the_midpoint_is_pinned() {
    const SkeletonColor from = SkeletonKey::presetWarm();
    const SkeletonColor to = SkeletonKey::presetCool();

    // weight 0 -> exactly `from`, weight 255 -> exactly `to`. Any rounding that
    // breaks these would make the slider fail to reach its own endpoints.
    TEST_ASSERT_EQUAL(from.red, SkeletonKey::blend(from, to, 0).red);
    TEST_ASSERT_EQUAL(from.green, SkeletonKey::blend(from, to, 0).green);
    TEST_ASSERT_EQUAL(from.blue, SkeletonKey::blend(from, to, 0).blue);
    TEST_ASSERT_EQUAL(to.red, SkeletonKey::blend(from, to, 255).red);
    TEST_ASSERT_EQUAL(to.green, SkeletonKey::blend(from, to, 255).green);
    TEST_ASSERT_EQUAL(to.blue, SkeletonKey::blend(from, to, 255).blue);

    // Midpoint of warm(255,180,110) -> cool(140,190,255) at weight 128. Pinned
    // because this is the number the on-device slider shows: host and firmware
    // must not diverge by a rounding step.
    const SkeletonColor mid = SkeletonKey::blend(from, to, 128);
    TEST_ASSERT_EQUAL(197, mid.red);
    TEST_ASSERT_EQUAL(185, mid.green);
    TEST_ASSERT_EQUAL(182, mid.blue);
}

// ---------------------------------------------------------------------------
// Device classification
// ---------------------------------------------------------------------------

void test_known_appearances_map_and_the_rest_stay_unknown() {
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Watch, SkeletonKey::classifyAppearance(0x0041));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Watch, SkeletonKey::classifyAppearance(0x0042));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Remote, SkeletonKey::classifyAppearance(0x0180));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Sensor, SkeletonKey::classifyAppearance(0x0309));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Keyboard, SkeletonKey::classifyAppearance(0x03C1));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Mouse, SkeletonKey::classifyAppearance(0x03C2));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Gamepad, SkeletonKey::classifyAppearance(0x03C4));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Light, SkeletonKey::classifyAppearance(0x0C40));

    // Unrecognised must be Unknown, never a guess -- 0x03C3 is a joystick and
    // 0x03C5 a digitizer, neither of which this build can name.
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Unknown, SkeletonKey::classifyAppearance(0x0000));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Unknown, SkeletonKey::classifyAppearance(0x03C0));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Unknown, SkeletonKey::classifyAppearance(0x03C3));
    TEST_ASSERT_EQUAL(SkeletonDeviceKind::Unknown, SkeletonKey::classifyAppearance(0x0D00));
}

void test_every_state_has_a_short_header_label() {
    TEST_ASSERT_EQUAL_STRING("IDLE", skeletonStateLabel(SkeletonState::Idle));
    TEST_ASSERT_EQUAL_STRING("SCANNING", skeletonStateLabel(SkeletonState::Scanning));
    TEST_ASSERT_EQUAL_STRING("CONNECTING", skeletonStateLabel(SkeletonState::Connecting));
    TEST_ASSERT_EQUAL_STRING("DISCOVERING", skeletonStateLabel(SkeletonState::Discovering));
    TEST_ASSERT_EQUAL_STRING("READY", skeletonStateLabel(SkeletonState::Ready));
    TEST_ASSERT_EQUAL_STRING("SENDING", skeletonStateLabel(SkeletonState::Sending));
    TEST_ASSERT_EQUAL_STRING("FAILED", skeletonStateLabel(SkeletonState::Failed));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_tree_counts_split_by_property);
    RUN_TEST(test_apply_names_fills_known_uuids_and_leaves_unknown_empty);
    RUN_TEST(test_apply_names_defers_to_a_name_the_device_supplied);
    RUN_TEST(test_begin_requires_confirmation_and_resets_prior_state);
    RUN_TEST(test_a_scan_that_finds_nothing_fails_rather_than_loitering);
    RUN_TEST(test_connect_failure_is_reported_with_a_reason);
    RUN_TEST(test_full_dump_is_refused_until_discovery_completes);
    RUN_TEST(test_stop_drops_the_tree_so_a_stale_dump_cannot_be_written);
    RUN_TEST(test_presets_and_payload_are_the_rgb_triples_the_controllers_expect);
    RUN_TEST(test_blend_endpoints_are_exact_and_the_midpoint_is_pinned);
    RUN_TEST(test_known_appearances_map_and_the_rest_stay_unknown);
    RUN_TEST(test_every_state_has_a_short_header_label);
    return UNITY_END();
}
