// BleLegacyDisplay contract -- B4, the legacy half of the BLE name library.
//
// What this suite exists to protect:
//
//   1. The panel budget. The old CYD rows hold 14 characters, and the previous
//      hand-written table had no way to prove it stayed inside that: nothing
//      read the table but the firmware. Now the clamp is host-testable, so
//      "no row overflows" is an assertion over the *whole* table rather than a
//      spot check.
//   2. One source of numbers. This module holds no data of its own; it resolves
//      through GattNameTable. The coverage test below pins that, so a private
//      copy cannot creep back in -- which is exactly how 0x1822 came to be
//      labelled "HTTP Proxy" in one table and "Pulse Oximeter" in the other.
//   3. Unknown stays unknown. A UUID with no name yields nullptr / false so the
//      caller prints the raw UUID. Inventing a plausible name is worse than
//      none, because a captured dump is evidence.

#include <unity.h>

#include <cstring>
#include <string>

#include "features/ble/BleLegacyDisplay.h"
#include "features/ble/GattNameTable.h"

namespace {

// Reads a row the way the legacy screen would.
std::string panelRow(uint16_t uuid) {
    char row[BleLegacyDisplay::kMaxNameChars + 1] = {};
    if (!BleLegacyDisplay::writeServiceName(uuid, row, sizeof(row))) {
        return "<unknown>";
    }
    return std::string(row);
}

}  // namespace

// ─── The budget ────────────────────────────────────────────────────────────

void test_every_service_row_fits_the_panel_budget() {
    const uint16_t count = GattNameTable::serviceCount();
    TEST_ASSERT_GREATER_THAN(0, count);

    for (uint16_t index = 0; index < count; ++index) {
        const uint16_t uuid = GattNameTable::serviceUuidAt(index);
        char row[BleLegacyDisplay::kMaxNameChars + 1] = {};
        TEST_ASSERT_TRUE_MESSAGE(
            BleLegacyDisplay::writeServiceName(uuid, row, sizeof(row)),
            GattNameTable::serviceNameAt(index));
        // The buffer is kMaxNameChars + 1, so a full row would be exactly the
        // budget; anything longer would have been cut, which is the point.
        TEST_ASSERT_LESS_OR_EQUAL(BleLegacyDisplay::kMaxNameChars, std::strlen(row));
    }
}

void test_the_budget_actually_bites() {
    // If every name already fit, the clamp would be untested decoration. The
    // SIG names and the vendor/member tags are far longer, so it bites: the
    // longest row is 0xFFF0's vendor string at 41 characters, three times the
    // budget. Pinned so shrinking the table cannot silently make this vacuous.
    const std::size_t longest = BleLegacyDisplay::longestServiceNameLength();
    TEST_ASSERT_GREATER_THAN(BleLegacyDisplay::kMaxNameChars, longest);
    TEST_ASSERT_EQUAL(41, longest);
    TEST_ASSERT_EQUAL(41, std::strlen(BleLegacyDisplay::serviceName(0xFFF0)));
}

void test_a_full_row_is_clamped_to_the_prefix_not_mangled() {
    // "Continuous Glucose Monitoring" is the longest canonical service name.
    TEST_ASSERT_EQUAL_STRING("Continuous Glucose Monitoring",
                             BleLegacyDisplay::serviceName(0x181F));
    TEST_ASSERT_EQUAL_STRING("Continuous Glu", panelRow(0x181F).c_str());
}

// ─── The drift bug this replaced ───────────────────────────────────────────

void test_the_two_tables_no_longer_disagree_about_0x1822() {
    // The legacy table said "HTTP Proxy" for 0x1822; the SIG assigns 0x1822 to
    // Pulse Oximeter and 0x1823 to HTTP Proxy. Pinned on both paths now, because
    // the whole point of B4 was that there is only one path left.
    TEST_ASSERT_EQUAL_STRING("Pulse Oximeter", BleLegacyDisplay::serviceName(0x1822));
    TEST_ASSERT_EQUAL_STRING("Pulse Oximeter", panelRow(0x1822).c_str());
    TEST_ASSERT_EQUAL_STRING("HTTP Proxy", panelRow(0x1823).c_str());
}

// ─── One source of numbers ─────────────────────────────────────────────────

void test_coverage_is_exactly_the_canonical_table() {
    // Every UUID the canonical table knows must render on the legacy path...
    const uint16_t count = GattNameTable::serviceCount();
    uint16_t rendered = 0;
    for (uint16_t index = 0; index < count; ++index) {
        char row[BleLegacyDisplay::kMaxNameChars + 1] = {};
        if (BleLegacyDisplay::writeServiceName(GattNameTable::serviceUuidAt(index),
                                               row, sizeof(row))) {
            ++rendered;
        }
    }
    TEST_ASSERT_EQUAL(count, rendered);

    // ...and a UUID it does not know must not be nameable here. This is the
    // assertion that would have failed if a private copy of the numbers had
    // been left behind with an entry the canonical table lacks.
    TEST_ASSERT_NULL(BleLegacyDisplay::serviceName(0x1234));
    TEST_ASSERT_NULL(BleLegacyDisplay::serviceName(0x0000));
    TEST_ASSERT_NULL(BleLegacyDisplay::serviceName(0xFFFF));
    TEST_ASSERT_EQUAL_STRING("<unknown>", panelRow(0x1234).c_str());

    // A characteristic UUID is not a service, so it must miss as well.
    TEST_ASSERT_NULL(BleLegacyDisplay::serviceName(0x2A19));
}

void test_the_member_uuid_keeps_its_provenance_at_the_data_layer() {
    // The panel row is 14 characters, so "(member)" cannot survive the clamp.
    // That is deliberate: provenance belongs to the table, whose consumer is a
    // dump, not to a row whose consumer is a 14-character display.
    TEST_ASSERT_EQUAL_STRING("Fast Pair service (member)",
                             BleLegacyDisplay::serviceName(0xFE2C));
    TEST_ASSERT_EQUAL_STRING("Fast Pair serv", panelRow(0xFE2C).c_str());
}

// ─── Buffer discipline ─────────────────────────────────────────────────────

void test_a_smaller_caller_buffer_is_honoured() {
    char small[5] = {};
    TEST_ASSERT_TRUE(BleLegacyDisplay::writeClampedName("Battery", small, sizeof(small)));
    TEST_ASSERT_EQUAL_STRING("Batt", small);
    TEST_ASSERT_EQUAL(4, std::strlen(small));
}

void test_degenerate_buffers_and_names_are_refused_not_overrun() {
    char row[BleLegacyDisplay::kMaxNameChars + 1];

    // Null or zero-capacity output: refused, and nothing written.
    TEST_ASSERT_FALSE(BleLegacyDisplay::writeClampedName("Battery", nullptr, 16));
    TEST_ASSERT_FALSE(BleLegacyDisplay::writeClampedName("Battery", row, 0));

    // Missing or empty name: refused. A caller must not get a "" that looks
    // like a successful lookup of a nameless service.
    std::memset(row, 'x', sizeof(row));
    TEST_ASSERT_FALSE(BleLegacyDisplay::writeClampedName(nullptr, row, sizeof(row)));
    TEST_ASSERT_EQUAL_CHAR('\0', row[0]);
    TEST_ASSERT_FALSE(BleLegacyDisplay::writeClampedName("", row, sizeof(row)));
    TEST_ASSERT_EQUAL_CHAR('\0', row[0]);
}

void test_an_unknown_uuid_leaves_the_buffer_empty() {
    char row[8] = {};
    TEST_ASSERT_FALSE(BleLegacyDisplay::writeServiceName(0x1234, row, sizeof(row)));
    TEST_ASSERT_EQUAL_STRING("", row);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_service_row_fits_the_panel_budget);
    RUN_TEST(test_the_budget_actually_bites);
    RUN_TEST(test_a_full_row_is_clamped_to_the_prefix_not_mangled);
    RUN_TEST(test_the_two_tables_no_longer_disagree_about_0x1822);
    RUN_TEST(test_coverage_is_exactly_the_canonical_table);
    RUN_TEST(test_the_member_uuid_keeps_its_provenance_at_the_data_layer);
    RUN_TEST(test_a_smaller_caller_buffer_is_honoured);
    RUN_TEST(test_degenerate_buffers_and_names_are_refused_not_overrun);
    RUN_TEST(test_an_unknown_uuid_leaves_the_buffer_empty);
    return UNITY_END();
}
