// HID reader tooling: the sweep scheduler and the end-of-transaction report.
//
// Two claims this suite exists to hold up:
//
//   * A pass that produced no notification never reads as "completed". That is
//     the whole point of the report -- a reader that silently ignored the
//     command must not look like a success.
//   * The Artemis core sweep finishes. It is deliberately split two commands per
//     connect, so the scheduler has to walk a 64-command sweep across 32
//     connects rather than issue them all into one connection and time out.

#include <unity.h>

#include <cstring>

#include "features/ble/HidReaderModel.h"

// ---------------------------------------------------------------------------
// Identification
// ---------------------------------------------------------------------------

void test_only_known_reader_services_are_classified() {
    TEST_ASSERT_EQUAL(HidReaderKind::Signo, HidReaderModel::classify(0xFD40));
    TEST_ASSERT_EQUAL(HidReaderKind::IclassSe, HidReaderModel::classify(0xFFF0));

    // Anything else is Unknown, never a guess -- a wrong reader name in a capture
    // log is worse than none.
    TEST_ASSERT_EQUAL(HidReaderKind::Unknown, HidReaderModel::classify(0x1234));
    TEST_ASSERT_EQUAL(HidReaderKind::Unknown, HidReaderModel::classify(0x180F));
    TEST_ASSERT_EQUAL(HidReaderKind::Unknown, HidReaderModel::classify(0x0000));
}

void test_reader_and_state_labels_are_covered() {
    TEST_ASSERT_EQUAL_STRING("HID Signo", HidReaderModel::kindLabel(HidReaderKind::Signo));
    TEST_ASSERT_EQUAL_STRING("iCLASS SE", HidReaderModel::kindLabel(HidReaderKind::IclassSe));
    TEST_ASSERT_EQUAL_STRING("Unknown reader",
                             HidReaderModel::kindLabel(HidReaderKind::Unknown));

    TEST_ASSERT_EQUAL_STRING("IDLE", HidReaderModel::stateLabel(HidTransactionState::Idle));
    TEST_ASSERT_EQUAL_STRING("SCANNING",
                             HidReaderModel::stateLabel(HidTransactionState::Scanning));
    TEST_ASSERT_EQUAL_STRING("CONNECTING",
                             HidReaderModel::stateLabel(HidTransactionState::Connecting));
    TEST_ASSERT_EQUAL_STRING("CAPTURING",
                             HidReaderModel::stateLabel(HidTransactionState::Capturing));
    TEST_ASSERT_EQUAL_STRING("COMPLETED",
                             HidReaderModel::stateLabel(HidTransactionState::Completed));
    TEST_ASSERT_EQUAL_STRING("FAILED", HidReaderModel::stateLabel(HidTransactionState::Failed));
}

// ---------------------------------------------------------------------------
// End-of-transaction reporting
// ---------------------------------------------------------------------------

void test_only_a_captured_reply_completes_a_pass() {
    const HidTransactionReport good = HidReaderModel::report(true, true, true);
    TEST_ASSERT_EQUAL(HidTransactionState::Completed, good.state);
    TEST_ASSERT_TRUE(good.completed);
    TEST_ASSERT_TRUE(good.replyCaptured);
    TEST_ASSERT_EQUAL_STRING("reply captured", good.reason);

    // Connected, sent, and the reader said nothing. Not a success.
    const HidTransactionReport mute = HidReaderModel::report(true, true, false);
    TEST_ASSERT_EQUAL(HidTransactionState::Failed, mute.state);
    TEST_ASSERT_FALSE(mute.completed);
    TEST_ASSERT_FALSE(mute.replyCaptured);
    TEST_ASSERT_EQUAL_STRING("reader produced no notification", mute.reason);

    // No connection means no result, even if something did arrive over the air.
    const HidTransactionReport unconnected = HidReaderModel::report(true, false, true);
    TEST_ASSERT_EQUAL(HidTransactionState::Failed, unconnected.state);
    TEST_ASSERT_FALSE(unconnected.completed);
    TEST_ASSERT_FALSE(unconnected.replyCaptured);
    TEST_ASSERT_EQUAL_STRING("connect failed", unconnected.reason);

    // Never asked to do anything, so nothing is claimed in either direction.
    const HidTransactionReport idle = HidReaderModel::report(false, false, false);
    TEST_ASSERT_EQUAL(HidTransactionState::Idle, idle.state);
    TEST_ASSERT_FALSE(idle.completed);
    TEST_ASSERT_FALSE(idle.replyCaptured);
    TEST_ASSERT_EQUAL_STRING("not requested", idle.reason);
}

// ---------------------------------------------------------------------------
// Sweep scheduler
// ---------------------------------------------------------------------------

void test_a_sweep_issues_at_most_two_commands_per_connect() {
    HidSweepScheduler scheduler;
    scheduler.begin(5);
    TEST_ASSERT_EQUAL(5, scheduler.totalCommands());
    TEST_ASSERT_EQUAL(0, scheduler.issuedCommands());
    TEST_ASSERT_EQUAL(0, scheduler.connectsUsed());
    TEST_ASSERT_FALSE(scheduler.exhausted());

    uint8_t batch[kHidSweepCommandsPerConnect] = {0, 0};
    TEST_ASSERT_EQUAL(2, scheduler.nextBatch(batch, sizeof(batch)));
    TEST_ASSERT_EQUAL(0, batch[0]);
    TEST_ASSERT_EQUAL(1, batch[1]);
    TEST_ASSERT_EQUAL(2, scheduler.issuedCommands());
    TEST_ASSERT_EQUAL(1, scheduler.connectsUsed());

    TEST_ASSERT_EQUAL(2, scheduler.nextBatch(batch, sizeof(batch)));
    TEST_ASSERT_EQUAL(2, batch[0]);
    TEST_ASSERT_EQUAL(3, batch[1]);
    TEST_ASSERT_EQUAL(2, scheduler.connectsUsed());

    // The last connect carries a short batch -- one command, not a padded two.
    TEST_ASSERT_EQUAL(1, scheduler.nextBatch(batch, sizeof(batch)));
    TEST_ASSERT_EQUAL(4, batch[0]);
    TEST_ASSERT_EQUAL(3, scheduler.connectsUsed());
    TEST_ASSERT_EQUAL(5, scheduler.issuedCommands());
    TEST_ASSERT_TRUE(scheduler.exhausted());

    // An exhausted sweep must not open another connection.
    TEST_ASSERT_EQUAL(0, scheduler.nextBatch(batch, sizeof(batch)));
    TEST_ASSERT_EQUAL(3, scheduler.connectsUsed());
}

void test_a_sixty_four_command_sweep_completes_across_thirty_two_connects() {
    // This is the shape of the Artemis core sweep: it has to finish rather than
    // run every command into one connection and time out.
    HidSweepScheduler scheduler;
    scheduler.begin(64);

    uint8_t batch[kHidSweepCommandsPerConnect] = {0, 0};
    uint8_t totalIssued = 0;
    uint8_t connects = 0;
    while (!scheduler.exhausted() && connects < 255) {
        const uint8_t written = scheduler.nextBatch(batch, sizeof(batch));
        if (written == 0) {
            break;
        }
        // Commands must come out in order, exactly once each.
        for (uint8_t index = 0; index < written; ++index) {
            TEST_ASSERT_EQUAL(totalIssued, batch[index]);
            ++totalIssued;
        }
        ++connects;
    }

    TEST_ASSERT_EQUAL(64, totalIssued);
    TEST_ASSERT_EQUAL(32, connects);
    TEST_ASSERT_EQUAL(32, scheduler.connectsUsed());
    TEST_ASSERT_TRUE(scheduler.exhausted());
}

void test_the_scheduler_refuses_a_call_that_cannot_hold_a_batch() {
    HidSweepScheduler scheduler;
    scheduler.begin(4);

    uint8_t batch[kHidSweepCommandsPerConnect] = {0, 0};
    TEST_ASSERT_EQUAL(0, scheduler.nextBatch(nullptr, 2));
    TEST_ASSERT_EQUAL(0, scheduler.nextBatch(batch, 0));
    // A refusal must not consume a command or burn a connection.
    TEST_ASSERT_EQUAL(0, scheduler.issuedCommands());
    TEST_ASSERT_EQUAL(0, scheduler.connectsUsed());

    // Capacity below the per-connect cap is honoured, not overrun.
    TEST_ASSERT_EQUAL(1, scheduler.nextBatch(batch, 1));
    TEST_ASSERT_EQUAL(1, scheduler.issuedCommands());
    TEST_ASSERT_EQUAL(1, scheduler.connectsUsed());
}

void test_a_zero_command_sweep_is_exhausted_from_the_start() {
    HidSweepScheduler scheduler;
    scheduler.begin(0);
    uint8_t batch[kHidSweepCommandsPerConnect] = {0, 0};
    TEST_ASSERT_TRUE(scheduler.exhausted());
    TEST_ASSERT_EQUAL(0, scheduler.nextBatch(batch, sizeof(batch)));
    TEST_ASSERT_EQUAL(0, scheduler.connectsUsed());

    // reset() must return the scheduler to a clean, reusable state.
    scheduler.begin(4);
    TEST_ASSERT_EQUAL(1, scheduler.nextBatch(batch, 1));
    scheduler.reset();
    TEST_ASSERT_EQUAL(0, scheduler.totalCommands());
    TEST_ASSERT_EQUAL(0, scheduler.issuedCommands());
    TEST_ASSERT_EQUAL(0, scheduler.connectsUsed());
    TEST_ASSERT_TRUE(scheduler.exhausted());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_only_known_reader_services_are_classified);
    RUN_TEST(test_reader_and_state_labels_are_covered);
    RUN_TEST(test_only_a_captured_reply_completes_a_pass);
    RUN_TEST(test_a_sweep_issues_at_most_two_commands_per_connect);
    RUN_TEST(test_a_sixty_four_command_sweep_completes_across_thirty_two_connects);
    RUN_TEST(test_the_scheduler_refuses_a_call_that_cannot_hold_a_batch);
    RUN_TEST(test_a_zero_command_sweep_is_exhausted_from_the_start);
    return UNITY_END();
}
