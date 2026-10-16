#include <array>
#include <unity.h>

#include "FakeFeatureCanvas.h"
#include "core/FeatureRunner.h"

namespace {
std::array<const char*, 16> events{};
size_t eventCount = 0;
bool routeResult = true;
bool probeResult = true;
bool preflightResult = true;
bool beginResult = true;
bool drawResult = false;
bool exitRequest = false;
int beginCount = 0;
int tickCount = 0;
int drawCount = 0;
int endCount = 0;

Authorization authorizeResult{AuthorizationCode::Allowed, PolicyAction::Proceed,
                              "allowed by test"};

void record(const char* event) {
    if (eventCount < events.size()) {
        events[eventCount++] = event;
    }
}

Authorization authorize(const FeatureDescriptor&) {
    record("authorize");
    return authorizeResult;
}

bool routeSupported(const FeatureDescriptor&) {
    record("route");
    return routeResult;
}

bool probe(PeripheralId) {
    record("probe");
    return probeResult;
}

bool preflight(const FeatureDescriptor&) {
    record("preflight");
    return preflightResult;
}

class FakeFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        ++beginCount;
        record("begin");
        return beginResult ? FeatureResult{FeatureResultCode::Completed, "started"}
                           : FeatureResult{FeatureResultCode::Fault, "begin failed"};
    }

    void tick() override {
        ++tickCount;
        record("tick");
    }

    void requestStop(StopReason) override {
        record("requestStop");
    }

    FeatureResult end() override {
        ++endCount;
        record("end");
        return {FeatureResultCode::Completed, "ended"};
    }

    bool draw(IFeatureCanvas& canvas) override {
        ++drawCount;
        record("draw");
        canvas.drawText(0, 0, "fake", 0xFFFF, 1);
        return drawResult;
    }

    bool exitRequested() const override { return exitRequest; }
};

IFeature* makeFakeFeature() {
    return new FakeFeature();
}

// Both entry points are in one registry: the passive case has to be reachable
// for the fail-closed test to mean anything, and a registry that contained only
// the offensive descriptor would make it pass for the wrong reason
// ("not registered" instead of "the feature ran").
const FeatureDescriptor kDescriptorTable[2] = {
    {FeatureId::WifiDeauth,
     CategoryId::Wifi,
     "Fake offensive",
     RiskClass::Offensive,
     {PeripheralId::Cc1101},
     1,
     {},
     0,
     "test",
     "/test",
     makeFakeFeature},
    {FeatureId::WifiScanner,
     CategoryId::Wifi,
     "Fake passive",
     RiskClass::Passive,
     {},
     0,
     {},
     0,
     "test",
     "/test",
     makeFakeFeature},
};

const FeatureRegistry kRegistry(kDescriptorTable, 2);

FeatureContext& testContext() {
    static FeatureContext context{};
    return context;
}

FeatureRunner makeRunner(FeatureRunnerHooks hooks) {
    return FeatureRunner(kRegistry, testContext(), hooks);
}

FeatureRunner makeRunner() {
    return makeRunner({authorize, routeSupported, probe, preflight});
}

void resetState() {
    events.fill(nullptr);
    eventCount = 0;
    routeResult = true;
    probeResult = true;
    preflightResult = true;
    beginResult = true;
    drawResult = false;
    exitRequest = false;
    beginCount = 0;
    tickCount = 0;
    drawCount = 0;
    endCount = 0;
    authorizeResult = {AuthorizationCode::Allowed, PolicyAction::Proceed, "allowed by test"};
    testContext().canvas = nullptr;
}

void test_runner_orders_preflight_and_lifecycle() {
    resetState();
    auto runner = makeRunner();
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, runner.start(FeatureId::WifiDeauth).code);
    runner.tick();
    runner.requestStop(StopReason::UserRequest);
    runner.requestStop(StopReason::UserRequest);
    TEST_ASSERT_EQUAL(1, beginCount);
    TEST_ASSERT_EQUAL(1, tickCount);
    TEST_ASSERT_EQUAL(1, endCount);
    TEST_ASSERT_EQUAL_STRING("authorize", events[0]);
    TEST_ASSERT_EQUAL_STRING("route", events[1]);
    TEST_ASSERT_EQUAL_STRING("probe", events[2]);
    TEST_ASSERT_EQUAL_STRING("preflight", events[3]);
    TEST_ASSERT_EQUAL_STRING("begin", events[4]);
    TEST_ASSERT_EQUAL_STRING("tick", events[5]);
}

void test_policy_block_prevents_begin() {
    resetState();
    authorizeResult = {AuthorizationCode::Blocked, PolicyAction::BlockedBlueTeam,
                       "Blue Team mode active - leave via Device Policy"};
    auto runner = makeRunner();
    const FeatureResult result = runner.start(FeatureId::WifiDeauth);
    TEST_ASSERT_EQUAL(FeatureResultCode::Blocked, result.code);
    TEST_ASSERT_EQUAL(0, beginCount);
    // The policy's own words, not a generic label. The caller shows this line,
    // and "Blue Team mode active" says something "blocked" does not.
    TEST_ASSERT_EQUAL_STRING("Blue Team mode active - leave via Device Policy",
                             result.detail);
}

// A bool return would have collapsed "offer the disclaimer" and "Blue Team
// refuses" into one indistinguishable answer. The action has to survive to the
// caller, because those are two different screens.
void test_the_policy_action_survives_to_the_caller() {
    resetState();
    authorizeResult = {AuthorizationCode::Blocked, PolicyAction::PromptValhalla,
                       "VALHALLA responsibility disclaimer required"};
    auto runner = makeRunner();
    const FeatureResult result = runner.start(FeatureId::WifiDeauth);
    TEST_ASSERT_EQUAL(FeatureResultCode::Blocked, result.code);
    TEST_ASSERT_EQUAL_STRING("VALHALLA responsibility disclaimer required", result.detail);
}

void test_begin_failure_calls_end_once() {
    resetState();
    beginResult = false;
    auto runner = makeRunner();
    TEST_ASSERT_EQUAL(FeatureResultCode::Fault, runner.start(FeatureId::WifiDeauth).code);
    TEST_ASSERT_EQUAL(1, beginCount);
    TEST_ASSERT_EQUAL(1, endCount);
}

// ─── The gate that used to be missing ──────────────────────────────────────
//
// AppKernel passed `{}` and a null authorize read as "allowed", so every
// offensive feature started without consulting policy at all. Fail closed: no
// hook means the offensive ones do not run.
void test_a_runner_without_an_authorization_hook_refuses_offensive() {
    resetState();
    auto runner = makeRunner({nullptr, routeSupported, probe, preflight});
    const FeatureResult result = runner.start(FeatureId::WifiDeauth);
    TEST_ASSERT_EQUAL(FeatureResultCode::Blocked, result.code);
    TEST_ASSERT_EQUAL_STRING("no authorization hook installed", result.detail);
    TEST_ASSERT_EQUAL(0, beginCount);
}

// ...but a machine with no policy store still has to be usable, so the same
// asymmetry PolicyService::legacyAllowsOffensive uses applies here.
void test_a_runner_without_an_authorization_hook_still_runs_passive() {
    resetState();
    auto runner = makeRunner({nullptr, routeSupported, probe, preflight});
    const FeatureResult result = runner.start(FeatureId::WifiScanner);
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, result.code);
    TEST_ASSERT_EQUAL(1, beginCount);
}

// ─── The render hook ───────────────────────────────────────────────────────

void test_the_feature_owns_the_screen_only_when_it_painted() {
    resetState();
    FakeFeatureCanvas canvas(240, 320, 8);
    testContext().canvas = &canvas;
    drawResult = true;

    auto runner = makeRunner();
    TEST_ASSERT_TRUE(runner.start(FeatureId::WifiDeauth).code == FeatureResultCode::Completed);
    TEST_ASSERT_FALSE(runner.ownsScreen());

    runner.tick();
    TEST_ASSERT_TRUE(runner.ownsScreen());
    TEST_ASSERT_EQUAL(1, drawCount);
    TEST_ASSERT_EQUAL_STRING("fake", canvas.textAt(0));
    // present() only when the feature actually painted: a sprite-backed canvas
    // would otherwise push whatever stale pixels it still held.
    TEST_ASSERT_EQUAL(1, canvas.presentCount);

    // The next frame declines the screen, and the previous frame's claim must
    // not survive it.
    drawResult = false;
    runner.tick();
    TEST_ASSERT_FALSE(runner.ownsScreen());
    TEST_ASSERT_EQUAL(2, drawCount);
    TEST_ASSERT_EQUAL(1, canvas.presentCount);

    runner.requestStop(StopReason::UserRequest);
    TEST_ASSERT_FALSE(runner.ownsScreen());
}

void test_a_headless_context_is_never_asked_to_draw() {
    resetState();
    auto runner = makeRunner();
    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, runner.start(FeatureId::WifiDeauth).code);
    runner.tick();
    TEST_ASSERT_EQUAL(0, drawCount);
    TEST_ASSERT_FALSE(runner.ownsScreen());
}

// ─── The feature's own exit signal ─────────────────────────────────────────

// The legacy screens carry `X::isExitRequested()`. It used to be consulted by a
// hand-written loop per menu entry; through the interface, one loop serves them
// all -- including entries added to the catalog later.
void test_a_feature_that_stops_itself_is_visible_to_its_owner() {
    resetState();
    auto runner = makeRunner();
    TEST_ASSERT_FALSE(runner.exitRequested());

    TEST_ASSERT_EQUAL(FeatureResultCode::Completed, runner.start(FeatureId::WifiDeauth).code);
    TEST_ASSERT_FALSE(runner.exitRequested());

    exitRequest = true;
    runner.tick();
    TEST_ASSERT_TRUE(runner.exitRequested());

    runner.requestStop(StopReason::UserRequest);
    // Nothing is running, so nothing is asking to stop.
    TEST_ASSERT_FALSE(runner.exitRequested());
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_runner_orders_preflight_and_lifecycle);
    RUN_TEST(test_policy_block_prevents_begin);
    RUN_TEST(test_the_policy_action_survives_to_the_caller);
    RUN_TEST(test_begin_failure_calls_end_once);
    RUN_TEST(test_a_runner_without_an_authorization_hook_refuses_offensive);
    RUN_TEST(test_a_runner_without_an_authorization_hook_still_runs_passive);
    RUN_TEST(test_the_feature_owns_the_screen_only_when_it_painted);
    RUN_TEST(test_a_headless_context_is_never_asked_to_draw);
    RUN_TEST(test_a_feature_that_stops_itself_is_visible_to_its_owner);
    return UNITY_END();
}
