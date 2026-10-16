// Closing the loop between a live BLE session and the two models that decide
// whether a switch is safe.
//
// FeatureSwitchLedger has always taken a *declared* `resourcesHeldAfterEnd`, and
// a declared zero is exactly what cannot catch a session that forgets to hand
// the stack back. This suite is about the seam that replaces the declaration
// with a reading: BleResourceState (what the transport is holding right now),
// profileFromMeasurement (that reading corrected into the ledger's input) and
// measureBleVisit / assessBleVisit (the heap cost, graded the same way the bench
// grades a build).
//
// The three shipped clients are driven here for real, through FakeBleCentral,
// so "every client's end() releases the controller" is a measurement rather
// than a claim made on their behalf.

#include <unity.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "FakeBleCentral.h"
#include "drivers/BleVisitAudit.h"
#include "features/ble/ChameleonUltraClient.h"
#include "features/ble/HidReaderClient.h"
#include "features/ble/SkeletonKeyClient.h"

namespace {

class ScriptedHeapProbe final : public IBleHeapProbe {
public:
    HeapSample current{};
    mutable uint32_t reads = 0;

    HeapSample sampleHeap() const override {
        ++reads;
        return current;
    }
};

HeapSample heapOf(uint32_t freeHeap, uint32_t largestBlock = 65536U) {
    HeapSample sample;
    sample.freeHeap = freeHeap;
    sample.minFreeHeap = freeHeap;
    sample.largestBlock = largestBlock;
    return sample;
}

HeapBaseline c5Baseline() {
    HeapBaseline baseline{};
    std::strncpy(baseline.board, "nm-cyd-c5", sizeof(baseline.board) - 1U);
    baseline.freeHeap = 143216U;
    baseline.minFreeHeap = 118944U;
    baseline.largestBlock = 65536U;
    return baseline;
}

BleAddress addressOf(const char* text) { return FakeBleCentral::parseAddress(text); }

// A session that takes the controller, scans, connects and subscribes -- and
// whose teardown is whatever the test chooses to do next. Modelling the "bad"
// case as a missing call rather than as a broken transport is the point: a
// leak the transport itself can see is the easy case, and the one worth pinning
// is the leak only the pair count can see.
struct ForgettingSession {
    IBleCentral& central;

    void open() {
        central.begin();
        central.startScan(0, false);
        central.connect(addressOf("AA:00:00:00:00:01"), 1000);
        const BleCharacteristicRef ref = BleCharacteristicRef::fromShort(0x180F, 0x2A19);
        central.subscribe(ref, nullptr, nullptr);
    }

    void releaseThroughTheTransport() { central.end(); }
};

}  // namespace

// ─── what the transport is holding ─────────────────────────────────────────

void test_a_released_controller_holds_nothing_and_says_so() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    BleVisitAudit audit(fake, probe);

    const BleResourceState state = audit.resources();
    TEST_ASSERT_FALSE(state.running);
    TEST_ASSERT_FALSE(state.connected);
    TEST_ASSERT_FALSE(state.scanning);
    TEST_ASSERT_EQUAL_UINT32(0, state.held());
    TEST_ASSERT_TRUE(state.released());
    TEST_ASSERT_TRUE(state.balanced());

    char line[96] = {};
    TEST_ASSERT_TRUE(state.describe(line, sizeof(line)));
    TEST_ASSERT_EQUAL_STRING("released", line);
}

void test_a_session_that_never_tore_down_holds_every_lease_it_took() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    BleVisitAudit audit(fake, probe);

    audit.beginVisit();
    ForgettingSession session{fake};
    session.open();
    // No matching endVisit(): that is what a feature which never reaches its own
    // end() looks like from here.

    const BleResourceState state = audit.resources();
    TEST_ASSERT_TRUE(state.running);
    TEST_ASSERT_TRUE(state.connected);
    TEST_ASSERT_TRUE(state.scanning);
    TEST_ASSERT_EQUAL_UINT8(1, state.subscriptions);
    // Three separate leases: the controller, the scan and the subscription are
    // taken and released separately, and so they leak separately.
    TEST_ASSERT_EQUAL_UINT32(3, state.held());
    TEST_ASSERT_FALSE(state.released());
    TEST_ASSERT_FALSE(state.balanced());
    TEST_ASSERT_FALSE(audit.clean());

    char line[96] = {};
    TEST_ASSERT_TRUE(state.describe(line, sizeof(line)));
    TEST_ASSERT_NOT_NULL(std::strstr(line, "running"));
    TEST_ASSERT_NOT_NULL(std::strstr(line, "1 subscription(s)"));
    TEST_ASSERT_NOT_NULL(std::strstr(line, "begin 1 / end 0"));
}

void test_an_unbalanced_visit_is_caught_even_when_the_transport_looks_clean() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    BleVisitAudit audit(fake, probe);

    audit.beginVisit();
    ForgettingSession session{fake};
    session.open();
    // Every accessor the transport exposes now reads clean, because the session
    // really did release it. Only the pair count disagrees -- and the transport
    // cannot tell, because begin() is idempotent in the stack.
    session.releaseThroughTheTransport();
    fake.end();

    const BleResourceState state = audit.resources();
    TEST_ASSERT_FALSE(state.connected);
    TEST_ASSERT_FALSE(state.scanning);
    TEST_ASSERT_EQUAL_UINT8(0, state.subscriptions);
    TEST_ASSERT_TRUE(state.running);
    TEST_ASSERT_EQUAL_UINT32(1, state.held());
    TEST_ASSERT_FALSE(state.released());
    TEST_ASSERT_FALSE(state.balanced());
    TEST_ASSERT_FALSE(audit.clean());
}

void test_every_subscription_counts_as_its_own_lease() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    BleVisitAudit audit(fake, probe);
    audit.beginVisit();

    fake.connect(addressOf("AA:00:00:00:00:01"), 1000);
    const BleCharacteristicRef refs[4] = {
        BleCharacteristicRef::fromShort(0x180F, 0x2A19),
        BleCharacteristicRef::fromShort(0x180F, 0x2A1A),
        BleCharacteristicRef::fromShort(0x180F, 0x2A1B),
        BleCharacteristicRef::fromShort(0x180F, 0x2A1C),
    };
    for (const BleCharacteristicRef& ref : refs) {
        TEST_ASSERT_EQUAL(BleStatus::Ok, fake.subscribe(ref, nullptr, nullptr));
    }

    // One for the controller (held both because the visit never ended and
    // because the link is up -- the same lease, counted once), plus one per
    // subscription. The count has to track them rather than saturate at
    // "something is wrong".
    TEST_ASSERT_EQUAL_UINT32(1U + 4U, audit.resources().held());
}

// ─── the declaration, corrected by the reading ─────────────────────────────

void test_a_declared_clean_profile_is_replaced_by_what_the_transport_reported() {
    // The shape the shipped table has: every BLE feature declaring that it holds
    // nothing after teardown.
    const FeatureProfile kScanner{"ble-scanner", true, 90U, 30U, 70U, 0U};
    const FeatureProfile kSkeleton{"skeleton-key", true, 160U, 50U, 130U, 0U};

    const FeatureProfile* declaredOrder[2] = {&kScanner, &kSkeleton};
    FeatureSwitchLedger ledger;
    // On paper the order is fine -- which is exactly why a declaration cannot be
    // the thing that catches a leak.
    TEST_ASSERT_TRUE(ledger.replay(declaredOrder, 2).clean());

    // Now measure it: the SkeletonKey visit left a subscription standing.
    BleResourceState measured{};
    measured.subscriptions = 1;
    const FeatureProfile skeletonMeasured = profileFromMeasurement(kSkeleton, measured);
    TEST_ASSERT_EQUAL_UINT32(1, skeletonMeasured.resourcesHeldAfterEnd);
    // The timing fields are the declaration's, unchanged: no transport reports
    // them, so they are not overwritten.
    TEST_ASSERT_EQUAL_UINT32(160U, skeletonMeasured.beginMs);
    TEST_ASSERT_EQUAL_UINT32(130U, skeletonMeasured.endMs);
    TEST_ASSERT_EQUAL_STRING("skeleton-key", skeletonMeasured.id);

    const FeatureProfile* measuredOrder[2] = {&kScanner, &skeletonMeasured};
    const SwitchIssue issue = ledger.replay(measuredOrder, 2);
    TEST_ASSERT_EQUAL(SwitchIssueCode::ResourceLeak, issue.code);
    // Attributed to the visit that caused it, with the count it was holding.
    TEST_ASSERT_EQUAL_STRING("skeleton-key", issue.feature);
    TEST_ASSERT_EQUAL_UINT32(1, issue.observedMs);
}

void test_every_permutation_of_measured_clean_profiles_is_still_clean() {
    const FeatureProfile kScanner{"ble-scanner", true, 90U, 30U, 70U, 0U};
    const FeatureProfile kSkeleton{"skeleton-key", true, 160U, 50U, 130U, 0U};
    const FeatureProfile kChameleon{"chameleon-ultra", true, 200U, 60U, 140U, 0U};
    const FeatureProfile kHid{"hid-reader", true, 180U, 45U, 100U, 0U};

    const BleResourceState released{};
    const FeatureProfile measured[4] = {
        profileFromMeasurement(kScanner, released),
        profileFromMeasurement(kSkeleton, released),
        profileFromMeasurement(kChameleon, released),
        profileFromMeasurement(kHid, released),
    };
    // The measured table agrees with the declaration, so it is still 0 — the
    // point of the permutation sweep is that the measurement did not *introduce*
    // an issue of its own.
    for (const FeatureProfile& profile : measured) {
        TEST_ASSERT_EQUAL_UINT32(0, profile.resourcesHeldAfterEnd);
    }

    const FeatureProfile* profiles[4] = {&measured[0], &measured[1], &measured[2], &measured[3]};
    FeatureSwitchLedger ledger;
    int permutations = 0;
    do {
        const SwitchIssue issue = ledger.replay(profiles, 4);
        TEST_ASSERT_TRUE_MESSAGE(issue.clean(), issue.detail);
        ++permutations;
    } while (std::next_permutation(profiles, profiles + 4));

    TEST_ASSERT_EQUAL(24, permutations);
}

// ─── the heap cost of a visit ──────────────────────────────────────────────

void test_a_visit_that_does_not_give_the_heap_back_is_not_clean() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    audit.beginVisit();
    ForgettingSession session{fake};
    session.open();
    session.releaseThroughTheTransport();
    // 4 KiB never came back.
    probe.current = heapOf(143216U - 4096U);
    audit.endVisit();

    const BleHeapBudget budget = audit.heapCost();
    TEST_ASSERT_EQUAL_UINT32(143216U, budget.beforeFreeHeap);
    TEST_ASSERT_EQUAL_UINT32(139120U, budget.afterFreeHeap);
    TEST_ASSERT_EQUAL_INT32(-4096, budget.leakedBytes);
    // 4096 / 143216 = 2.86 %, so 286 hundredths.
    TEST_ASSERT_EQUAL_INT32(286, budget.percentDeltaX100);
    TEST_ASSERT_TRUE(audit.resources().released());
    // Every lease is back and the visit is still not clean: two independent
    // facts, and both are required.
    TEST_ASSERT_FALSE(audit.clean());
}

void test_a_visit_that_frees_memory_reports_a_negative_regression() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    probe.current = heapOf(120000U);
    BleVisitAudit audit(fake, probe);

    audit.beginVisit();
    probe.current = heapOf(130000U);
    audit.endVisit();

    const BleHeapBudget budget = audit.heapCost();
    TEST_ASSERT_EQUAL_INT32(10000, budget.leakedBytes);
    TEST_ASSERT_EQUAL_INT32(-833, budget.percentDeltaX100);
    TEST_ASSERT_TRUE(audit.clean());

    char line[96] = {};
    TEST_ASSERT_TRUE(audit.describe(line, sizeof(line)));
    TEST_ASSERT_EQUAL_STRING("held=0 heapDelta=10000B (-833 x100)", line);
}

void test_an_unfinished_visit_has_not_cost_anything_yet() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    audit.beginVisit();
    TEST_ASSERT_EQUAL_INT32(0, audit.heapCost().leakedBytes);
    TEST_ASSERT_EQUAL_UINT32(143216U, audit.heapCost().beforeFreeHeap);
    TEST_ASSERT_EQUAL_UINT32(143216U, audit.heapCost().afterFreeHeap);
    TEST_ASSERT_EQUAL_UINT32(1U, audit.visitCount());
    TEST_ASSERT_EQUAL_UINT16(1, audit.resources().opens);
    TEST_ASSERT_EQUAL_UINT16(0, audit.resources().closes);
}

void test_a_shrinking_heap_across_visits_is_visible_as_a_trend() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    int32_t previous = 0;
    for (int visit = 0; visit < 3; ++visit) {
        audit.beginVisit();
        const uint32_t before = probe.current.freeHeap;
        // Each visit loses 8 KiB that never comes back.
        probe.current = heapOf(before - 8192U);
        audit.endVisit();

        const BleHeapBudget budget = audit.heapCost();
        TEST_ASSERT_EQUAL_INT32(-8192, budget.leakedBytes);
        // The same byte count each time is a larger fraction of a smaller heap,
        // which is why the trend is not flat and why a per-visit constant is
        // not a sufficient budget.
        TEST_ASSERT_GREATER_THAN((int)previous, (int)budget.percentDeltaX100);
        previous = budget.percentDeltaX100;
        TEST_ASSERT_FALSE(audit.clean());
    }
    TEST_ASSERT_EQUAL_UINT32(3U, audit.visitCount());

    // 24576 bytes of 143216 is 17.2 %, past the critical line.
    const HeapAssessment assessment = assessBleVisit(c5Baseline(), probe.current);
    TEST_ASSERT_EQUAL(HeapVerdict::Critical, assessment.verdict);
}

void test_the_visit_cost_and_the_bench_verdict_agree_about_the_same_number() {
    const HeapBaseline reference = c5Baseline();

    // Identical sample: nothing to report.
    const HeapSample same = heapOf(reference.freeHeap);
    const HeapAssessment unchanged = assessBleVisit(reference, same);
    TEST_ASSERT_EQUAL(HeapVerdict::WithinBudget, unchanged.verdict);
    TEST_ASSERT_EQUAL_INT32(0, measureBleVisit(same, same).percentDeltaX100);

    // 9 % down: past the 8 % warn line, short of the 15 % critical one.
    const HeapSample warned = heapOf(130327U);
    const HeapAssessment warn = assessBleVisit(reference, warned);
    TEST_ASSERT_EQUAL(HeapVerdict::Warn, warn.verdict);
    TEST_ASSERT_EQUAL_INT32(warn.percentDeltaX100,
                            measureBleVisit(same, warned).percentDeltaX100);

    // 16 % down: critical.
    const HeapSample regressed = heapOf(120301U);
    TEST_ASSERT_EQUAL(HeapVerdict::Critical, assessBleVisit(reference, regressed).verdict);

    // Below the absolute floor the relative number is irrelevant.
    const HeapSample starved = heapOf(20000U, 8192U);
    TEST_ASSERT_EQUAL(HeapVerdict::Critical, assessBleVisit(reference, starved).verdict);

    // A baseline with no board tag is unusable rather than merely bad.
    HeapBaseline unnamed{};
    unnamed.freeHeap = 143216U;
    unnamed.largestBlock = 65536U;
    TEST_ASSERT_EQUAL(HeapVerdict::Invalid, assessBleVisit(unnamed, same).verdict);
}

void test_a_line_that_does_not_fit_is_refused_rather_than_truncated() {
    FakeBleCentral fake;
    ScriptedHeapProbe probe;
    BleVisitAudit audit(fake, probe);
    audit.beginVisit();

    // Room for a terminator but nothing else: the call fails and the buffer is
    // left empty, so a caller cannot mistake a cut line for a complete one.
    char one[1] = {'X'};
    TEST_ASSERT_FALSE(audit.describe(one, sizeof(one)));
    TEST_ASSERT_EQUAL_CHAR('\0', one[0]);

    char tiny[4] = {'X', 'X', 'X', '\0'};
    TEST_ASSERT_FALSE(audit.resources().describe(tiny, sizeof(tiny)));
    TEST_ASSERT_EQUAL_CHAR('\0', tiny[0]);

    TEST_ASSERT_FALSE(audit.describe(nullptr, 32));
}

// ─── every shipped client hands the controller back ────────────────────────

void test_the_skeleton_key_client_releases_the_controller() {
    FakeBleCentral fake;
    fake.addAdvertisement("AA:00:00:00:00:01", -40, "LED-1");
    fake.addService(0xFFF0);
    fake.addCharacteristic(0, 0xFFF3, kBlePropWrite | kBlePropWriteNoResponse);

    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    SkeletonKeyClient client(fake);
    audit.beginVisit();
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_TRUE(client.beginSession(true));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.scan(false));
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.connectToStrongest(1000));
    TEST_ASSERT_TRUE(fake.connected());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());
    audit.endVisit();

    const BleResourceState state = audit.resources();
    char line[96] = {};
    state.describe(line, sizeof(line));
    TEST_ASSERT_TRUE_MESSAGE(state.released(), line);
    TEST_ASSERT_TRUE(state.balanced());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
    TEST_ASSERT_TRUE(audit.clean());
}

void test_the_chameleon_client_releases_the_controller_and_its_subscription() {
    FakeBleCentral fake;
    fake.addService(ChameleonUltraClient::serviceUuid());
    fake.addCharacteristic(0, ChameleonUltraClient::rxUuid(),
                           kBlePropWrite | kBlePropWriteNoResponse);
    fake.addCharacteristic(0, ChameleonUltraClient::txUuid(), kBlePropNotify);

    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    ChameleonUltraClient client(fake);
    audit.beginVisit();
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.connect(addressOf("AA:00:00:00:00:01"), 1000));
    TEST_ASSERT_TRUE(client.linkReady());
    TEST_ASSERT_EQUAL(1, fake.subscriptionCount());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());
    audit.endVisit();

    const BleResourceState state = audit.resources();
    TEST_ASSERT_EQUAL_UINT8(0, state.subscriptions);
    TEST_ASSERT_EQUAL_UINT32(0, state.held());
    TEST_ASSERT_TRUE(state.balanced());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
    TEST_ASSERT_TRUE(audit.clean());
}

void test_the_hid_reader_client_releases_the_controller_and_its_subscription() {
    FakeBleCentral fake;
    BleAdvertisement& advertisement = fake.addAdvertisement("AA:00:00:00:00:F0", -50, "reader");
    advertisement.serviceUuids[0] = BleUuid::fromShort(0xFFF0);
    advertisement.serviceCount = 1;
    fake.addService(0xFFF0);
    fake.addCharacteristic(0, 0xFFF1, kBlePropWrite);
    fake.addCharacteristic(0, 0xFFF2, kBlePropNotify);

    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    HidReaderClient client(fake);
    audit.beginVisit();
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.passiveScan());
    HidDiagnostic diagnostic;
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.diagnose(0, diagnostic));
    TEST_ASSERT_EQUAL(1, fake.subscriptionCount());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.end());
    audit.endVisit();

    const BleResourceState state = audit.resources();
    TEST_ASSERT_EQUAL_UINT8(0, state.subscriptions);
    TEST_ASSERT_EQUAL_UINT32(0, state.held());
    TEST_ASSERT_TRUE(state.balanced());
    TEST_ASSERT_EQUAL(1, fake.endCalls);
    TEST_ASSERT_TRUE(audit.clean());
    TEST_ASSERT_EQUAL_UINT16(1, audit.visitCount());
}

void test_a_client_that_is_not_ended_is_the_leak_the_ledger_reports() {
    FakeBleCentral fake;
    fake.addService(ChameleonUltraClient::serviceUuid());
    fake.addCharacteristic(0, ChameleonUltraClient::rxUuid(), kBlePropWrite);
    fake.addCharacteristic(0, ChameleonUltraClient::txUuid(), kBlePropNotify);

    ScriptedHeapProbe probe;
    probe.current = heapOf(143216U);
    BleVisitAudit audit(fake, probe);

    ChameleonUltraClient client(fake);
    audit.beginVisit();
    TEST_ASSERT_TRUE(client.begin());
    TEST_ASSERT_EQUAL(BleStatus::Ok, client.connect(addressOf("AA:00:00:00:00:01"), 1000));
    // The feature is switched away from without ever reaching end().
    audit.endVisit();

    const BleResourceState state = audit.resources();
    TEST_ASSERT_EQUAL_UINT8(1, state.subscriptions);
    TEST_ASSERT_TRUE(state.connected);
    TEST_ASSERT_FALSE(state.released());
    TEST_ASSERT_EQUAL(0, fake.endCalls);

    const FeatureProfile declared{"chameleon-ultra", true, 170U, 60U, 140U, 0U};
    const FeatureProfile measured = profileFromMeasurement(declared, state);
    TEST_ASSERT_EQUAL_UINT32(state.held(), measured.resourcesHeldAfterEnd);
    TEST_ASSERT_TRUE(measured.resourcesHeldAfterEnd > 0U);

    const FeatureProfile* order[1] = {&measured};
    FeatureSwitchLedger ledger;
    const SwitchIssue issue = ledger.replay(order, 1);
    TEST_ASSERT_EQUAL(SwitchIssueCode::ResourceLeak, issue.code);
    TEST_ASSERT_EQUAL_STRING("chameleon-ultra", issue.feature);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_a_released_controller_holds_nothing_and_says_so);
    RUN_TEST(test_a_session_that_never_tore_down_holds_every_lease_it_took);
    RUN_TEST(test_an_unbalanced_visit_is_caught_even_when_the_transport_looks_clean);
    RUN_TEST(test_every_subscription_counts_as_its_own_lease);
    RUN_TEST(test_a_declared_clean_profile_is_replaced_by_what_the_transport_reported);
    RUN_TEST(test_every_permutation_of_measured_clean_profiles_is_still_clean);
    RUN_TEST(test_a_visit_that_does_not_give_the_heap_back_is_not_clean);
    RUN_TEST(test_a_visit_that_frees_memory_reports_a_negative_regression);
    RUN_TEST(test_an_unfinished_visit_has_not_cost_anything_yet);
    RUN_TEST(test_a_shrinking_heap_across_visits_is_visible_as_a_trend);
    RUN_TEST(test_the_visit_cost_and_the_bench_verdict_agree_about_the_same_number);
    RUN_TEST(test_a_line_that_does_not_fit_is_refused_rather_than_truncated);
    RUN_TEST(test_the_skeleton_key_client_releases_the_controller);
    RUN_TEST(test_the_chameleon_client_releases_the_controller_and_its_subscription);
    RUN_TEST(test_the_hid_reader_client_releases_the_controller_and_its_subscription);
    RUN_TEST(test_a_client_that_is_not_ended_is_the_leak_the_ledger_reports);
    return UNITY_END();
}
