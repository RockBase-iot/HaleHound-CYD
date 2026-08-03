#include <unity.h>

#include "services/PeripheralRegistry.h"

namespace {
class FakeDriver final : public IPeripheralDriver {
public:
    explicit FakeDriver(PeripheralId peripheral, ProbeResult result)
        : peripheral_(peripheral), result_(result) {}

    PeripheralId id() const override { return peripheral_; }
    ProbeResult probe(const ExpansionRoute&, uint32_t) override {
        ++probeCount;
        return result_;
    }

    PeripheralId peripheral_;
    ProbeResult result_;
    int probeCount = 0;
};
}

void test_registry_defaults_to_unverified_and_missing_route_is_unsupported() {
    PeripheralRegistry registry(BoardId::NmCydC5);
    TEST_ASSERT_EQUAL(PeripheralStatus::Unverified,
                      registry.status(PeripheralId::Cc1101));
    TEST_ASSERT_EQUAL(PeripheralStatus::Unsupported,
                      registry.ensureAvailable(PeripheralId::IrTx).status);
}

void test_registry_probes_on_demand_and_caches_result() {
    PeripheralRegistry registry(BoardId::NmCydC5);
    FakeDriver driver(PeripheralId::Pn532,
                      {PeripheralStatus::Available, "pn532 ready"});
    registry.registerDriver(driver);

    const auto first = registry.ensureAvailable(PeripheralId::Pn532);
    const auto second = registry.ensureAvailable(PeripheralId::Pn532);

    TEST_ASSERT_EQUAL(PeripheralStatus::Available, first.status);
    TEST_ASSERT_EQUAL(PeripheralStatus::Available, second.status);
    TEST_ASSERT_EQUAL(1, driver.probeCount);
}

void test_registry_keeps_device_failures_independent() {
    PeripheralRegistry registry(BoardId::NmCydC5);
    FakeDriver missing(PeripheralId::Cc1101,
                       {PeripheralStatus::NotFound, "cc1101 absent"});
    FakeDriver ready(PeripheralId::Nrf24,
                     {PeripheralStatus::Available, "nrf24 ready"});
    registry.registerDriver(missing);
    registry.registerDriver(ready);

    TEST_ASSERT_EQUAL(PeripheralStatus::NotFound,
                      registry.ensureAvailable(PeripheralId::Cc1101).status);
    TEST_ASSERT_EQUAL(PeripheralStatus::Available,
                      registry.ensureAvailable(PeripheralId::Nrf24).status);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_defaults_to_unverified_and_missing_route_is_unsupported);
    RUN_TEST(test_registry_probes_on_demand_and_caches_result);
    RUN_TEST(test_registry_keeps_device_failures_independent);
    return UNITY_END();
}
