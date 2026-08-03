#include <cstdint>
#include <string>
#include <unity.h>
#include <vector>

#include "features/drone/RemoteIdParser.h"

void test_remote_id_parser_accepts_documented_fixture_and_merges_transports() {
    const std::vector<uint8_t> fixture = {
        0x00, 0x08, 'D', 'R', 'O', 'N', 'E', '-', '0', '1',
        0x01, 0x2A, 0x00, 0x00, 0x00, 0x02, 0x4F, 0x00, 0x00, 0x00};
    const auto wifi = RemoteIdParser::parse(fixture, RemoteIdTransport::Wifi);
    const auto ble = RemoteIdParser::parse(fixture, RemoteIdTransport::Ble);
    TEST_ASSERT_TRUE(wifi.valid);
    TEST_ASSERT_EQUAL_STRING("DRONE-01", wifi.record.serial.c_str());
    TEST_ASSERT_TRUE(wifi.record.hasLocation);
    RemoteIdAggregator aggregator;
    aggregator.observe(wifi.record);
    aggregator.observe(ble.record);
    TEST_ASSERT_EQUAL(1, aggregator.size());
    TEST_ASSERT_TRUE(aggregator.find("DRONE-01").transportMask == 3);
}

void test_remote_id_parser_rejects_short_or_unknown_frames() {
    const auto shortFrame = RemoteIdParser::parse({0x00, 0x08, 'D'}, RemoteIdTransport::Wifi);
    TEST_ASSERT_FALSE(shortFrame.valid);
    const auto unknown = RemoteIdParser::parse({0x7F, 0x00, 0x00, 0x00}, RemoteIdTransport::Ble);
    TEST_ASSERT_FALSE(unknown.valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_remote_id_parser_accepts_documented_fixture_and_merges_transports);
    RUN_TEST(test_remote_id_parser_rejects_short_or_unknown_frames);
    return UNITY_END();
}
