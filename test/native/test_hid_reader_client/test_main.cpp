// HID reader tooling over a scripted BLE controller.
//
// The model suite (test_hid_reader) already covers the sweep scheduler and the
// transaction report on their own. This suite covers the part that has a radio
// on one side and an unverified protocol on the other -- and the only thing
// interesting about that boundary is what the tool is allowed to *claim*.
//
// Three claims are pinned here, each one a place where a plausible-looking
// implementation would be lying:
//
//   1. A match on 0xFFF0 is reported as colliding with a role the SIG's own
//      list already assigns that number to (Triones / ELK-BLEDOM LED
//      controllers). It is never reported as a verified reader.
//   2. A reader that offers no notify characteristic, or no write target, is
//      refused rather than driven into a void and timed out.
//   3. Commands go to the characteristic the property bits say accepts a write,
//      not to whatever happens to be subscribed.
//
// The opcodes themselves are absent on purpose: this build has no verified
// Signo / iCLASS SE command set, so the suite supplies its own and only checks
// the batching and the accounting around them.

#include <unity.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "FakeBleCentral.h"
#include "features/ble/HidReaderClient.h"

namespace {

// 0xFD40 is what this build matches a Signo on; 0xFFF0 what it matches an
// iCLASS SE on. Neither has an authoritative source (REMAINING_WORK X3).
constexpr uint16_t kSignoService = 0xFD40;
constexpr uint16_t kIclassService = 0xFFF0;

BleAddress addressOf(const char* text) {
    return FakeBleCentral::parseAddress(text);
}

// Adds an advertisement carrying exactly the services given. The returned
// reference is only used immediately: a later add invalidates it.
void addAdvertisementWithService(FakeBleCentral& fake, const char* address, int8_t rssi,
                                 const char* name, uint16_t service, bool connectable = true) {
    BleAdvertisement& advertisement = fake.addAdvertisement(address, rssi, name, connectable);
    advertisement.serviceUuids[0] = BleUuid::fromShort(service);
    advertisement.serviceCount = 1;
}

void addAdvertisementWithService(FakeBleCentral& fake, const char* address, int8_t rssi,
                                 const char* name, const BleUuid& service,
                                 bool connectable = true) {
    BleAdvertisement& advertisement = fake.addAdvertisement(address, rssi, name, connectable);
    advertisement.serviceUuids[0] = service;
    advertisement.serviceCount = 1;
}

const char* signoAdvertisementNumber() { return "AA:00:00:00:00:40"; }

// A reader that exposes one writable and two notifying characteristics, which
// is the shape the diagnostic is specified against.
void scriptNotifyingReader(FakeBleCentral& fake) {
    fake.addService(kIclassService);
    fake.addCharacteristic(0, 0xFFF1, kBlePropWrite | kBlePropWriteNoResponse);
    fake.addCharacteristic(0, 0xFFF2, kBlePropNotify);
    fake.addCharacteristic(0, 0xFFF3, kBlePropIndicate | kBlePropRead);
}

BleCharacteristicRef writeRefOfReader() {
    return BleCharacteristicRef::fromShort(kIclassService, 0xFFF1);
}

BleCharacteristicRef notifyRefOfReader() {
    return BleCharacteristicRef::fromShort(kIclassService, 0xFFF2);
}

}  // namespace

// ─── passive scan ──────────────────────────────────────────────────────────

void test_a_reader_is_recognised_by_its_service_uuid_but_never_as_verified() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, signoAdvertisementNumber(), -50, "Signo", kSignoService);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    TEST_ASSERT_EQUAL_UINT8(1, client.candidateCount());

    const HidReaderCandidate& candidate = client.candidates()[0];
    TEST_ASSERT_EQUAL(HidReaderKind::Signo, candidate.kind);
    TEST_ASSERT_EQUAL_UINT16(kSignoService, candidate.matchedServiceUuid);
    TEST_ASSERT_EQUAL_STRING("Signo", candidate.name.c_str());
    TEST_ASSERT_EQUAL_STRING("AA:00:00:00:00:40", candidate.addressText.c_str());
    // The mapping has no authoritative source, so the claim travels with the
    // flag rather than living in a comment (X3).
    TEST_ASSERT_FALSE(candidate.mappingVerified);
    // 0xFD40 is not in the SIG table this project ships, so nothing collides.
    TEST_ASSERT_FALSE(candidate.uuidCollidesWithAnotherRole);
}

void test_a_uuid_that_the_sig_table_gives_another_role_is_flagged_as_colliding() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -60, "reader?", kIclassService);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    TEST_ASSERT_EQUAL_UINT8(1, client.candidateCount());

    const HidReaderCandidate& candidate = client.candidates()[0];
    TEST_ASSERT_EQUAL(HidReaderKind::IclassSe, candidate.kind);
    // 0xFFF0 is the Triones / ELK-BLEDOM LED service as far as the SIG list is
    // concerned. Reporting the reader match without that would overstate it.
    TEST_ASSERT_TRUE(candidate.uuidCollidesWithAnotherRole);
    TEST_ASSERT_FALSE(candidate.mappingVerified);
}

void test_a_128_bit_vendor_uuid_is_not_squeezed_into_a_16_bit_match() {
    FakeBleCentral fake;
    // Nordic's UART service: the Chameleon link, not a reader, and not on the
    // SIG base -- so it has no 16-bit form at all.
    BleUuid uart;
    TEST_ASSERT_TRUE(BleUuid::fromText("6E400001-B5A3-F393-E0A9-E50E24DCCA9E", uart));
    addAdvertisementWithService(fake, "AA:00:00:00:00:01", -55, "chameleon", uart);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    // A client that forced the 128-bit value through shortValue() would get 0,
    // and a client that matched on the last two bytes would invent a reader.
    TEST_ASSERT_EQUAL_UINT8(0, client.candidateCount());
}

void test_a_peer_advertising_nothing_this_build_knows_is_not_listed_as_a_reader() {
    FakeBleCentral fake;
    // Battery service: a legitimate device, not a reader.
    addAdvertisementWithService(fake, "AA:00:00:00:00:0F", -45, "battery", 0x180F);
    // No service list at all.
    fake.addAdvertisement("AA:00:00:00:00:10", -70, "anonymous");

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    TEST_ASSERT_EQUAL_UINT8(0, client.candidateCount());
}

void test_a_non_connectable_reader_advertisement_is_skipped() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:40", -30, "Signo beacon", kSignoService,
                                /*connectable=*/false);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    // Advertising the service and refusing a connection is not a reader this
    // tool can diagnose, so it must not be offered as one.
    TEST_ASSERT_EQUAL_UINT8(0, client.candidateCount());
}

void test_a_repeated_reader_is_one_candidate_and_the_list_is_capped() {
    FakeBleCentral fake;
    // The same reader advertising twice, which a real scan does constantly.
    addAdvertisementWithService(fake, "AA:00:00:00:00:40", -50, "Signo", kSignoService);
    addAdvertisementWithService(fake, "AA:00:00:00:00:40", -48, "Signo", kSignoService);
    // And enough distinct readers to overflow the cap.
    for (int index = 0; index < 20; ++index) {
        char text[18] = {};
        std::snprintf(text, sizeof(text), "BB:00:00:00:00:%02X", index);
        addAdvertisementWithService(fake, text, -60, nullptr, kSignoService);
    }

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    // One row per address, bounded: a scan list that grows with the number of
    // advertisements is a heap exhaustion with a UI attached.
    TEST_ASSERT_EQUAL_UINT8(16, client.candidateCount());
    TEST_ASSERT_EQUAL_STRING("AA:00:00:00:00:40", client.candidates()[0].addressText.c_str());
}

void test_the_passive_scan_asks_the_peer_for_nothing() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, signoAdvertisementNumber(), -50, "Signo", kSignoService);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    // A reader that wants to stay quiet answers a scan request with silence, and
    // a detection would then be something the tool provoked instead of
    // something the reader said. The transport is the only place that shows.
    TEST_ASSERT_EQUAL(0, fake.activeScanRequests);
    TEST_ASSERT_EQUAL(1, fake.passiveScanRequests);
    TEST_ASSERT_FALSE(fake.scanning());
}

// ─── connect diagnostic ────────────────────────────────────────────────────

void test_a_diagnostic_reports_what_the_reader_exposes_without_claiming_the_reply_characteristic() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());

    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    TEST_ASSERT_TRUE(diagnostic.connectSucceeded);
    TEST_ASSERT_EQUAL_UINT16(1, diagnostic.serviceCount);
    TEST_ASSERT_EQUAL_UINT16(3, diagnostic.characteristicCount);
    // Two characteristics can carry a reply. Which one the reader's protocol
    // names is exactly what has no source, so the count is reported instead of
    // an assertion (X3).
    TEST_ASSERT_EQUAL_UINT16(2, diagnostic.notifyCharacteristicCount);
    TEST_ASSERT_EQUAL_UINT16(1, diagnostic.writeCharacteristicCount);
    TEST_ASSERT_TRUE(diagnostic.subscribed);
    TEST_ASSERT_TRUE(diagnostic.writableFound);
    // Nothing has arrived yet, and a healthy reader says nothing until it has
    // something to say -- so "no reply" here is the normal state, not a failure.
    TEST_ASSERT_FALSE(diagnostic.replyCaptured);
    TEST_ASSERT_FALSE(client.replyCaptured());
    TEST_ASSERT_EQUAL(1, fake.subscriptionCount());
}

void test_a_reader_with_nothing_to_notify_on_is_refused_rather_than_listened_to() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "writer", kIclassService);
    fake.addService(kIclassService);
    fake.addCharacteristic(0, 0xFFF1, kBlePropWrite);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());

    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::NoService, client.diagnose(0, diagnostic));
    TEST_ASSERT_TRUE(diagnostic.connectSucceeded);
    TEST_ASSERT_EQUAL_UINT16(0, diagnostic.notifyCharacteristicCount);
    TEST_ASSERT_EQUAL_UINT16(1, diagnostic.writeCharacteristicCount);
    TEST_ASSERT_FALSE(diagnostic.subscribed);
    // The failure names the half that is missing, so the screen can say why
    // instead of "no reply" when nothing was ever going to answer.
    TEST_ASSERT_NOT_NULL(std::strstr(diagnostic.detail, "notify"));
    // Nothing was subscribed, so nothing is held.
    TEST_ASSERT_EQUAL(0, fake.subscriptionCount());
}

void test_a_reader_with_nothing_to_write_to_is_refused_rather_than_swept_into_the_void() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "listener", kIclassService);
    fake.addService(kIclassService);
    fake.addCharacteristic(0, 0xFFF2, kBlePropNotify);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());

    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::NoService, client.diagnose(0, diagnostic));
    TEST_ASSERT_TRUE(diagnostic.subscribed);
    TEST_ASSERT_FALSE(diagnostic.writableFound);
    TEST_ASSERT_NOT_NULL(std::strstr(diagnostic.detail, "write"));
    // The subscription taken before the shortfall was noticed is still live;
    // end() is what releases it, and that is asserted separately.
}

void test_a_failed_connect_is_reported_as_its_own_failure_not_as_a_diagnostic() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);
    fake.connectResult = BleStatus::ConnectFailed;

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());

    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::ConnectFailed, client.diagnose(0, diagnostic));
    TEST_ASSERT_FALSE(diagnostic.connectSucceeded);
    TEST_ASSERT_EQUAL_STRING(bleStatusLabel(BleStatus::ConnectFailed), diagnostic.detail);
    TEST_ASSERT_EQUAL(0, fake.subscriptionCount());
}

void test_diagnosing_an_index_that_is_not_in_the_list_is_not_found() {
    FakeBleCentral fake;
    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());

    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::NotFound, client.diagnose(0, diagnostic));
    TEST_ASSERT_FALSE(diagnostic.connectSucceeded);
    TEST_ASSERT_EQUAL(0, fake.connectCalls);
}

// ─── the reply, and what it promotes ───────────────────────────────────────

void test_a_notification_on_the_subscribed_characteristic_is_the_readers_reply() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    TEST_ASSERT_FALSE(client.report(/*requested=*/true).completed);

    const uint8_t reply[] = {0xAB, 0xCD, 0x01};
    TEST_ASSERT_EQUAL_UINT8(1, fake.deliver(notifyRefOfReader(), reply, sizeof(reply)));

    TEST_ASSERT_TRUE(client.replyCaptured());
    TEST_ASSERT_EQUAL_UINT32(1, client.notificationsSeen());
    TEST_ASSERT_EQUAL_UINT16(3, client.lastNotification().size());
    TEST_ASSERT_EQUAL_HEX8(0xAB, client.lastNotification()[0]);

    const HidTransactionReport report = client.report(/*requested=*/true);
    // Only a reply promotes the pass. A connect alone never did.
    TEST_ASSERT_TRUE(report.completed);
    TEST_ASSERT_EQUAL(HidTransactionState::Completed, report.state);
}

void test_a_notification_on_another_characteristic_is_not_the_readers_reply() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));

    // The second notifying characteristic of the same service. A transport that
    // matched on the service would deliver this; a real stack would not.
    const uint8_t reply[] = {0x01};
    const BleCharacteristicRef other = BleCharacteristicRef::fromShort(kIclassService, 0xFFF3);
    TEST_ASSERT_EQUAL_UINT8(0, fake.deliver(other, reply, sizeof(reply)));

    TEST_ASSERT_FALSE(client.replyCaptured());
    TEST_ASSERT_EQUAL_UINT32(0, client.notificationsSeen());
    TEST_ASSERT_FALSE(client.report(/*requested=*/true).completed);
}

void test_a_pass_the_user_never_requested_never_reports_as_completed() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    const uint8_t reply[] = {0x01};
    TEST_ASSERT_EQUAL_UINT8(1, fake.deliver(notifyRefOfReader(), reply, sizeof(reply)));

    const HidTransactionReport unrequested = client.report(/*requested=*/false);
    TEST_ASSERT_FALSE(unrequested.completed);
    TEST_ASSERT_EQUAL(HidTransactionState::Idle, unrequested.state);
    TEST_ASSERT_EQUAL_STRING("not requested", unrequested.reason);
    // The same session, this time asked for, does complete -- so the rule is
    // the request, not the session.
    TEST_ASSERT_TRUE(client.report(/*requested=*/true).completed);
}

void test_a_diagnostic_clears_a_reply_captured_by_the_previous_one() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    const uint8_t reply[] = {0x01};
    fake.deliver(notifyRefOfReader(), reply, sizeof(reply));
    TEST_ASSERT_TRUE(client.replyCaptured());

    // A second diagnostic is a new pass. Carrying the old reply forward would
    // report a reply the reader never sent this time.
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    TEST_ASSERT_FALSE(client.replyCaptured());
    TEST_ASSERT_EQUAL_UINT32(0, client.notificationsSeen());
    TEST_ASSERT_TRUE(client.lastNotification().empty());
    TEST_ASSERT_FALSE(client.report(/*requested=*/true).completed);
}

// ─── the sweep ─────────────────────────────────────────────────────────────

struct SweepFixture {
    FakeBleCentral fake;
    HidReaderClient client{fake};

    bool prepare() {
        addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
        scriptNotifyingReader(fake);
        if (!client.begin()) {
            return false;
        }
        if (client.passiveScan() != BleStatus::Ok) {
            return false;
        }
        HidDiagnostic diagnostic;
        return client.diagnose(0, diagnostic) == BleStatus::Ok;
    }
};

void test_the_sweep_batches_across_connects_and_writes_to_the_write_characteristic() {
    SweepFixture fixture;
    TEST_ASSERT_TRUE(fixture.prepare());

    // Five commands of two bytes; the scheduler may only issue two per connect.
    const uint8_t commands[] = {0xA0, 0xA1, 0xB0, 0xB1, 0xC0, 0xC1, 0xD0, 0xD1, 0xE0, 0xE1};
    const uint8_t reply[] = {0x00};
    for (int index = 0; index < 5; ++index) {
        fixture.fake.queueReply(writeRefOfReader(), notifyRefOfReader(),
                                std::vector<uint8_t>(reply, reply + sizeof(reply)));
    }

    HidSweepResult result;
    TEST_ASSERT_EQUAL(BleStatus::Ok, fixture.client.sweep(commands, 2, 5, result));
    TEST_ASSERT_EQUAL_UINT8(5, result.totalCommands);
    TEST_ASSERT_EQUAL_UINT8(5, result.commandsIssued);
    // 5 commands at 2 per connect: 2 + 2 + 1.
    TEST_ASSERT_EQUAL_UINT8(3, result.connectsUsed);
    TEST_ASSERT_EQUAL_UINT8(3, result.connectsWithReply);
    TEST_ASSERT_TRUE(result.complete);

    TEST_ASSERT_EQUAL_UINT32(5, fixture.fake.writeCount());
    for (std::size_t index = 0; index < 5; ++index) {
        // The write target comes from the property bits, not from being the
        // subscribed characteristic.
        TEST_ASSERT_TRUE(fixture.fake.writeAt(index).ref == writeRefOfReader());
        TEST_ASSERT_TRUE(fixture.fake.writeAt(index).data ==
                         std::vector<uint8_t>(commands + index * 2, commands + index * 2 + 2));
    }
}

void test_a_sweep_that_hears_nothing_is_not_a_completed_sweep() {
    SweepFixture fixture;
    TEST_ASSERT_TRUE(fixture.prepare());

    const uint8_t commands[] = {0xA0, 0xA1, 0xB0, 0xB1};
    // No replies scripted: the writes are accepted and answered by nobody.
    HidSweepResult result;
    TEST_ASSERT_EQUAL(BleStatus::Ok, fixture.client.sweep(commands, 2, 2, result));
    // Every command went out, and the pass still did not complete: issuing is
    // not the same as being heard, and a tool that conflated them would report
    // a dead reader as a finished sweep.
    TEST_ASSERT_EQUAL_UINT8(2, result.commandsIssued);
    TEST_ASSERT_EQUAL_UINT8(0, result.connectsWithReply);
    TEST_ASSERT_FALSE(result.complete);
}

void test_a_sweep_with_no_commands_is_not_a_completed_sweep() {
    SweepFixture fixture;
    TEST_ASSERT_TRUE(fixture.prepare());

    HidSweepResult result;
    TEST_ASSERT_EQUAL(BleStatus::Ok, fixture.client.sweep(nullptr, 0, 0, result));
    TEST_ASSERT_FALSE(result.complete);
    TEST_ASSERT_EQUAL_UINT8(0, result.commandsIssued);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.fake.writeCount());
}

void test_a_sweep_without_a_connection_is_refused_before_it_writes_anything() {
    FakeBleCentral fake;
    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());

    const uint8_t commands[] = {0xA0, 0xA1};
    HidSweepResult result;
    TEST_ASSERT_EQUAL(BleStatus::Disconnected, client.sweep(commands, 2, 1, result));
    TEST_ASSERT_EQUAL_UINT32(0, fake.writeCount());
    TEST_ASSERT_FALSE(result.complete);
}

void test_a_refused_write_stops_the_sweep_with_the_counters_left_where_they_stand() {
    SweepFixture fixture;
    TEST_ASSERT_TRUE(fixture.prepare());
    fixture.fake.writeResult = BleStatus::NotPermitted;

    const uint8_t commands[] = {0xA0, 0xA1, 0xB0, 0xB1, 0xC0, 0xC1};
    HidSweepResult result;
    // The peer refusing a write is a fact about the reader, and it is reported
    // as itself rather than as a completed sweep with fewer commands.
    TEST_ASSERT_EQUAL(BleStatus::NotPermitted, fixture.client.sweep(commands, 2, 3, result));
    TEST_ASSERT_EQUAL_UINT8(0, result.commandsIssued);
    TEST_ASSERT_EQUAL_UINT8(0, result.connectsWithReply);
    TEST_ASSERT_FALSE(result.complete);
    // The connect was used: the caller can see the pass stopped part way.
    TEST_ASSERT_EQUAL_UINT8(1, result.connectsUsed);
}

// ─── teardown ──────────────────────────────────────────────────────────────

void test_end_releases_the_controller_and_the_subscription() {
    FakeBleCentral fake;
    addAdvertisementWithService(fake, "AA:00:00:00:00:F0", -50, "reader", kIclassService);
    scriptNotifyingReader(fake);

    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    TEST_ASSERT_TRUE(fake.connected());
    TEST_ASSERT_EQUAL(1, fake.subscriptionCount());

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());
    // A subscription left live and a controller left up are the white-screen
    // cause the ledger models. A session that only dropped its own pointers
    // would pass every other test in this file.
    TEST_ASSERT_EQUAL(0, fake.subscriptionCount());
    TEST_ASSERT_FALSE(fake.connected());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
    TEST_ASSERT_EQUAL_UINT8(0, client.candidateCount());
}

void test_end_after_a_scan_that_found_nothing_still_releases_the_controller() {
    FakeBleCentral fake;
    HidReaderClient client(fake);
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());

    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());
    TEST_ASSERT_FALSE(fake.scanning());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
}

void test_a_sweep_after_end_is_refused_rather_than_writing_through_a_dead_controller() {
    SweepFixture fixture;
    TEST_ASSERT_TRUE(fixture.prepare());
    TEST_ASSERT_EQUAL(BleStatus::Ok, fixture.client.end());

    const uint8_t commands[] = {0xA0, 0xA1};
    HidSweepResult result;
    TEST_ASSERT_NOT_EQUAL(BleStatus::Ok, fixture.client.sweep(commands, 2, 1, result));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.fake.writeCount());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_a_reader_is_recognised_by_its_service_uuid_but_never_as_verified);
    RUN_TEST(test_a_uuid_that_the_sig_table_gives_another_role_is_flagged_as_colliding);
    RUN_TEST(test_a_128_bit_vendor_uuid_is_not_squeezed_into_a_16_bit_match);
    RUN_TEST(test_a_peer_advertising_nothing_this_build_knows_is_not_listed_as_a_reader);
    RUN_TEST(test_a_non_connectable_reader_advertisement_is_skipped);
    RUN_TEST(test_a_repeated_reader_is_one_candidate_and_the_list_is_capped);
    RUN_TEST(test_the_passive_scan_asks_the_peer_for_nothing);
    RUN_TEST(test_a_diagnostic_reports_what_the_reader_exposes_without_claiming_the_reply_characteristic);
    RUN_TEST(test_a_reader_with_nothing_to_notify_on_is_refused_rather_than_listened_to);
    RUN_TEST(test_a_reader_with_nothing_to_write_to_is_refused_rather_than_swept_into_the_void);
    RUN_TEST(test_a_failed_connect_is_reported_as_its_own_failure_not_as_a_diagnostic);
    RUN_TEST(test_diagnosing_an_index_that_is_not_in_the_list_is_not_found);
    RUN_TEST(test_a_notification_on_the_subscribed_characteristic_is_the_readers_reply);
    RUN_TEST(test_a_notification_on_another_characteristic_is_not_the_readers_reply);
    RUN_TEST(test_a_pass_the_user_never_requested_never_reports_as_completed);
    RUN_TEST(test_a_diagnostic_clears_a_reply_captured_by_the_previous_one);
    RUN_TEST(test_the_sweep_batches_across_connects_and_writes_to_the_write_characteristic);
    RUN_TEST(test_a_sweep_that_hears_nothing_is_not_a_completed_sweep);
    RUN_TEST(test_a_sweep_with_no_commands_is_not_a_completed_sweep);
    RUN_TEST(test_a_sweep_without_a_connection_is_refused_before_it_writes_anything);
    RUN_TEST(test_a_refused_write_stops_the_sweep_with_the_counters_left_where_they_stand);
    RUN_TEST(test_end_releases_the_controller_and_the_subscription);
    RUN_TEST(test_end_after_a_scan_that_found_nothing_still_releases_the_controller);
    RUN_TEST(test_a_sweep_after_end_is_refused_rather_than_writing_through_a_dead_controller);
    return UNITY_END();
}
