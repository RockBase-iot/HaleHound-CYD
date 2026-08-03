#include <unity.h>

#include "platform/ExecutionPolicy.h"

void test_classic_radio_can_use_pinned_task() {
    const WorkloadSpec radio{WorkloadRole::Radio, 4, 2, 6144, true};
    TEST_ASSERT_EQUAL(
        ExecutionMode::PinnedTask,
        chooseMode(BoardId::Esp32Cyd, radio));
}

void test_c5_radio_uses_cooperative_tick() {
    const WorkloadSpec radio{WorkloadRole::Radio, 4, 2, 6144, true};
    TEST_ASSERT_EQUAL(
        ExecutionMode::CooperativeTick,
        chooseMode(BoardId::NmCydC5, radio));
}

void test_c5_ui_uses_cooperative_tick() {
    const WorkloadSpec ui{WorkloadRole::Ui, 10, 4, 4096, false};
    TEST_ASSERT_EQUAL(
        ExecutionMode::CooperativeTick,
        chooseMode(BoardId::NmCydC5, ui));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_classic_radio_can_use_pinned_task);
    RUN_TEST(test_c5_radio_uses_cooperative_tick);
    RUN_TEST(test_c5_ui_uses_cooperative_tick);
    return UNITY_END();
}
