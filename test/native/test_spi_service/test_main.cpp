#include <array>
#include <unity.h>

#include "services/SpiBusService.h"

namespace {
std::array<const char*, 16> events{};
size_t eventCount = 0;

void record(const char* event) {
    if (eventCount < events.size()) {
        events[eventCount++] = event;
    }
}

class FakeBackend final : public ISpiBackend {
public:
    bool lock(uint32_t) override {
        record("lock");
        return true;
    }
    void unlock() override { record("unlock"); }
    void deselectAll() override { record("deselect_all"); }
    bool beginTransaction(SpiDeviceId) override {
        record("begin_transaction");
        return true;
    }
    bool select(SpiDeviceId) override {
        record("select_target");
        return true;
    }
    uint8_t transfer(uint8_t value) override {
        record("transfer");
        return value;
    }
    void deselect(SpiDeviceId) override { record("deselect_target"); }
    void endTransaction() override { record("end_transaction"); }
};

bool transferCallback(ISpiBackend& backend) {
    backend.transfer(0x5a);
    return true;
}

bool failingCallback(ISpiBackend&) {
    record("callback_failure");
    return false;
}

void resetEvents() {
    events.fill(nullptr);
    eventCount = 0;
}

void test_spi_transaction_has_complete_cleanup_order() {
    resetEvents();
    ResourceBroker broker;
    FakeBackend backend;
    SpiBusService service(broker, backend);
    const auto result = service.transact(SpiDeviceId::Display, transferCallback, 0);
    TEST_ASSERT_EQUAL(SpiResultCode::Completed, result.code);
    TEST_ASSERT_EQUAL_STRING("lock", events[0]);
    TEST_ASSERT_EQUAL_STRING("deselect_all", events[1]);
    TEST_ASSERT_EQUAL_STRING("begin_transaction", events[2]);
    TEST_ASSERT_EQUAL_STRING("select_target", events[3]);
    TEST_ASSERT_EQUAL_STRING("transfer", events[4]);
    TEST_ASSERT_EQUAL_STRING("deselect_target", events[5]);
    TEST_ASSERT_EQUAL_STRING("end_transaction", events[6]);
    TEST_ASSERT_EQUAL_STRING("unlock", events[7]);
}

void test_spi_transaction_cleans_up_after_callback_failure() {
    resetEvents();
    ResourceBroker broker;
    FakeBackend backend;
    SpiBusService service(broker, backend);
    const auto result = service.transact(SpiDeviceId::Cc1101, failingCallback, 0);
    TEST_ASSERT_EQUAL(SpiResultCode::Fault, result.code);
    TEST_ASSERT_EQUAL_STRING("deselect_target", events[5]);
    TEST_ASSERT_EQUAL_STRING("end_transaction", events[6]);
    TEST_ASSERT_EQUAL_STRING("unlock", events[7]);
}

void test_all_shared_devices_use_shared_spi_resource() {
    TEST_ASSERT_EQUAL(ResourceId::SharedSpi,
                      SpiBusService::resourceFor(SpiDeviceId::Display));
    TEST_ASSERT_EQUAL(ResourceId::SharedSpi,
                      SpiBusService::resourceFor(SpiDeviceId::Touch));
    TEST_ASSERT_EQUAL(ResourceId::SharedSpi,
                      SpiBusService::resourceFor(SpiDeviceId::Sd));
    TEST_ASSERT_EQUAL(ResourceId::SharedSpi,
                      SpiBusService::resourceFor(SpiDeviceId::Cc1101));
    TEST_ASSERT_EQUAL(ResourceId::SharedSpi,
                      SpiBusService::resourceFor(SpiDeviceId::Nrf24));
}

void test_legacy_session_holds_resource_until_deselect() {
    resetEvents();
    ResourceBroker broker;
    FakeBackend backend;
    SpiBusService service(broker, backend);
    SpiBusService competingService(broker, backend);

    TEST_ASSERT_EQUAL(SpiResultCode::Completed,
                      service.beginLegacySession(SpiDeviceId::Sd, 0, false).code);
    TEST_ASSERT_TRUE(service.hasLegacySession());
    TEST_ASSERT_EQUAL(SpiResultCode::Busy,
                      competingService.beginLegacySession(SpiDeviceId::Nrf24, 0, false).code);
    TEST_ASSERT_EQUAL(SpiResultCode::Completed, service.endLegacySession().code);
    TEST_ASSERT_FALSE(service.hasLegacySession());
    TEST_ASSERT_EQUAL_STRING("deselect_target", events[3]);
    TEST_ASSERT_EQUAL_STRING("unlock", events[4]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_spi_transaction_has_complete_cleanup_order);
    RUN_TEST(test_spi_transaction_cleans_up_after_callback_failure);
    RUN_TEST(test_all_shared_devices_use_shared_spi_resource);
    RUN_TEST(test_legacy_session_holds_resource_until_deselect);
    return UNITY_END();
}
}
