#include <unity.h>

#include "services/ResourceBroker.h"

void test_shared_resource_is_exclusive_and_release_is_reusable() {
    ResourceBroker broker;
    auto first = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::Display, 0);
    TEST_ASSERT_TRUE(first.acquired());
    auto second = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::SdCard, 0);
    TEST_ASSERT_EQUAL(AcquireCode::Busy, second.code());
    first.release();
    TEST_ASSERT_TRUE(
        broker.tryAcquire(ResourceId::SharedSpi, OwnerId::SdCard, 0).acquired());
}

void test_different_resources_can_be_held_in_parallel() {
    ResourceBroker broker;
    auto spi = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::Display, 0);
    auto display = broker.tryAcquire(ResourceId::Display, OwnerId::Display, 0);
    TEST_ASSERT_TRUE(spi.acquired());
    TEST_ASSERT_TRUE(display.acquired());
}

void test_recursive_ownership_is_rejected_and_timeout_is_reported() {
    ResourceBroker broker;
    auto first = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::Display, 0);
    auto recursive = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::Display, 0);
    TEST_ASSERT_EQUAL(AcquireCode::RecursiveDenied, recursive.code());
    auto timeout = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::SdCard, 25);
    TEST_ASSERT_EQUAL(AcquireCode::Timeout, timeout.code());
}

void test_recovery_releases_stale_lease() {
    ResourceBroker broker;
    auto first = broker.tryAcquire(ResourceId::SharedSpi, OwnerId::Display, 0);
    TEST_ASSERT_TRUE(first.acquired());
    TEST_ASSERT_TRUE(broker.recover(ResourceId::SharedSpi));
    TEST_ASSERT_TRUE(
        broker.tryAcquire(ResourceId::SharedSpi, OwnerId::SdCard, 0).acquired());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_shared_resource_is_exclusive_and_release_is_reusable);
    RUN_TEST(test_different_resources_can_be_held_in_parallel);
    RUN_TEST(test_recursive_ownership_is_rejected_and_timeout_is_reported);
    RUN_TEST(test_recovery_releases_stale_lease);
    return UNITY_END();
}
