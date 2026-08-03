#include <array>
#include <unity.h>

#include "core/FeatureRunner.h"

namespace {
std::array<const char*, 8> events{};
size_t eventCount = 0;
bool authorizeResult = true;
bool routeResult = true;
bool probeResult = true;
bool preflightResult = true;
bool beginResult = true;
int beginCount = 0;
int tickCount = 0;
int endCount = 0;

void record(const char* event) {
    if (eventCount < events.size()) {
        events[eventCount++] = event;
    }
}

bool authorize(const FeatureDescriptor&) {
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
};

IFeature* makeFakeFeature() {
    return new FakeFeature();
}

FeatureRunner makeRunner() {
    static const FeatureDescriptor descriptor{
        FeatureId::WifiDeauth,
        CategoryId::Wifi,
        "Fake",
        RiskClass::Offensive,
        {PeripheralId::Cc1101},
        1,
        {},
        0,
        "test",
        "/test",
        makeFakeFeature,
    };
    static const FeatureRegistry registry(&descriptor, 1);
    static FeatureContext context{};
    return FeatureRunner(registry, context, {authorize, routeSupported, probe, preflight});
}

void resetState() {
    events.fill(nullptr);
    eventCount = 0;
    authorizeResult = true;
    routeResult = true;
    probeResult = true;
    preflightResult = true;
    beginResult = true;
    beginCount = 0;
    tickCount = 0;
    endCount = 0;
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
    authorizeResult = false;
    auto runner = makeRunner();
    TEST_ASSERT_EQUAL(FeatureResultCode::Blocked, runner.start(FeatureId::WifiDeauth).code);
    TEST_ASSERT_EQUAL(0, beginCount);
}

void test_begin_failure_calls_end_once() {
    resetState();
    beginResult = false;
    auto runner = makeRunner();
    TEST_ASSERT_EQUAL(FeatureResultCode::Fault, runner.start(FeatureId::WifiDeauth).code);
    TEST_ASSERT_EQUAL(1, beginCount);
    TEST_ASSERT_EQUAL(1, endCount);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_runner_orders_preflight_and_lifecycle);
    RUN_TEST(test_policy_block_prevents_begin);
    RUN_TEST(test_begin_failure_calls_end_once);
    return UNITY_END();
}
