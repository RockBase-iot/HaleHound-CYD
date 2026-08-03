#include <unity.h>

#include "board/BoardProfile.h"
#include "board/ExpansionRoute.h"

void test_classic_board_profile_is_fixed() {
    const auto& classic = getBoardProfile(BoardId::Esp32Cyd);
    TEST_ASSERT_EQUAL(BoardId::Esp32Cyd, classic.id);
    TEST_ASSERT_EQUAL_UINT32(4U * 1024U * 1024U, classic.flashBytes);
    TEST_ASSERT_FALSE(classic.singleCore);
}

void test_c5_board_profile_uses_shared_spi() {
    const auto& c5 = getBoardProfile(BoardId::NmCydC5);
    TEST_ASSERT_EQUAL(BoardId::NmCydC5, c5.id);
    TEST_ASSERT_EQUAL_UINT32(16U * 1024U * 1024U, c5.flashBytes);
    TEST_ASSERT_TRUE(c5.singleCore);
    TEST_ASSERT_EQUAL(6, c5.sharedSpi.sck);
    TEST_ASSERT_EQUAL(2, c5.sharedSpi.miso);
    TEST_ASSERT_EQUAL(7, c5.sharedSpi.mosi);
}

void test_c5_pn532_route_is_i2c() {
    TEST_ASSERT_EQUAL(
        Transport::I2c,
        findRoute(BoardId::NmCydC5, PeripheralId::Pn532).transport);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_classic_board_profile_is_fixed);
    RUN_TEST(test_c5_board_profile_uses_shared_spi);
    RUN_TEST(test_c5_pn532_route_is_i2c);
    return UNITY_END();
}
