// GattNameTable contract.
//
// Two things this suite exists to protect:
//
//   1. The normaliser. Every SIG UUID is `xxxxxxxx-0000-1000-8000-00805f9b34fb`,
//      so four different spellings of the same UUID must all land on the same
//      short value -- and a UUID that is *not* on the base must not be forced
//      into one, because that would print a confident wrong name next to real
//      captured data.
//   2. The tables. They are linearly scanned, so a duplicated or out-of-order
//      entry is silent: lookup() just stops finding the later copy. The counts
//      are pinned so that editing a table has to be a deliberate act.

#include <unity.h>

#include <string>

#include "features/ble/GattNameTable.h"

// ---------------------------------------------------------------------------
// Normalisation
// ---------------------------------------------------------------------------

void test_every_spelling_of_a_sig_uuid_collapses_to_the_same_short_form() {
    uint16_t value = 0;

    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("180F", value));
    TEST_ASSERT_EQUAL(0x180F, value);

    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("180f", value));
    TEST_ASSERT_EQUAL(0x180F, value);

    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("0x180F", value));
    TEST_ASSERT_EQUAL(0x180F, value);

    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("0000180F", value));
    TEST_ASSERT_EQUAL(0x180F, value);

    // The canonical 128-bit form, which is what a real GATT dump contains.
    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("0000180f-0000-1000-8000-00805f9b34fb", value));
    TEST_ASSERT_EQUAL(0x180F, value);

    // Separators must not change the meaning.
    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("0000:180F", value));
    TEST_ASSERT_EQUAL(0x180F, value);

    TEST_ASSERT_EQUAL_STRING("0000-1000-8000-00805f9b34fb", GattNameTable::baseUuidSuffix());
}

void test_vendor_uuids_on_the_sig_base_collapse_too() {
    // The RGB-controller UUIDs are ordinary SIG-base UUIDs with an unassigned
    // 16-bit field, so they must fall out of the same code path.
    uint16_t value = 0;
    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("0000fff0-0000-1000-8000-00805f9b34fb", value));
    TEST_ASSERT_EQUAL(0xFFF0, value);
    TEST_ASSERT_TRUE(GattNameTable::toShortUuid("0000fff3-0000-1000-8000-00805f9b34fb", value));
    TEST_ASSERT_EQUAL(0xFFF3, value);
}

void test_a_uuid_off_the_sig_base_is_not_forced_into_a_short_form() {
    uint16_t value = 0xDEAD;

    // A genuine 128-bit vendor UUID.
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("12345678-1234-5678-1234-56789abcdef0", value));

    // A 32-bit UUID whose high half is non-zero cannot be a 16-bit UUID.
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("12345678", value));

    // 128 bits that differ from the base in the low 96 bits.
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("0000180f-0000-1000-8000-00805f9b34fc", value));

    // On failure `out` must be left untouched, not half-written.
    TEST_ASSERT_EQUAL(0xDEAD, value);
}

void test_malformed_uuids_are_rejected_rather_than_guessed() {
    uint16_t value = 0;
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("", value));
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("180", value));      // 3 nibbles
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("180F0", value));    // 5 nibbles
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("180G", value));     // not hex
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("0x", value));       // prefix only
    // Longer than 128 bits.
    TEST_ASSERT_FALSE(GattNameTable::toShortUuid("0000180f00001000800000805f9b34fb00", value));
}

// ---------------------------------------------------------------------------
// Table integrity
// ---------------------------------------------------------------------------

void test_tables_are_sorted_and_the_counts_are_pinned() {
    // Sortedness is what makes the linear scan safe: a duplicated key would make
    // the second copy permanently unreachable without failing anything.
    TEST_ASSERT_TRUE(GattNameTable::tablesAreSorted());

    // Pinned so that adding or dropping an entry is a conscious change. Update
    // the number here and say why, rather than letting it drift. Services went
    // 44 -> 75 in B4, when this table absorbed the legacy service table's rows
    // (see test_the_legacy_service_rows_are_now_covered_here below).
    TEST_ASSERT_EQUAL(75, GattNameTable::serviceCount());
    TEST_ASSERT_EQUAL(106, GattNameTable::characteristicCount());
    TEST_ASSERT_EQUAL(15, GattNameTable::descriptorCount());
}

// ---------------------------------------------------------------------------
// Lookups
// ---------------------------------------------------------------------------

void test_well_known_names_resolve_and_unknown_stays_empty() {
    TEST_ASSERT_EQUAL_STRING("GAP", GattNameTable::serviceName(0x1800));
    TEST_ASSERT_EQUAL_STRING("Device Information", GattNameTable::serviceName(0x180A));
    TEST_ASSERT_EQUAL_STRING("Battery", GattNameTable::serviceName(0x180F));
    TEST_ASSERT_EQUAL_STRING("Human Interface Device", GattNameTable::serviceName(0x1812));
    TEST_ASSERT_EQUAL_STRING("Environmental Sensing", GattNameTable::serviceName(0x181A));

    TEST_ASSERT_EQUAL_STRING("Device Name", GattNameTable::characteristicName(0x2A00));
    TEST_ASSERT_EQUAL_STRING("Battery Level", GattNameTable::characteristicName(0x2A19));
    TEST_ASSERT_EQUAL_STRING("Manufacturer Name String", GattNameTable::characteristicName(0x2A29));
    TEST_ASSERT_EQUAL_STRING("Report", GattNameTable::characteristicName(0x2A4D));
    TEST_ASSERT_EQUAL_STRING("Temperature", GattNameTable::characteristicName(0x2A6E));

    TEST_ASSERT_EQUAL_STRING("Characteristic User Description",
                             GattNameTable::descriptorName(0x2901));
    TEST_ASSERT_EQUAL_STRING("Client Characteristic Configuration",
                             GattNameTable::descriptorName(0x2902));

    // Unknown is empty, never invented -- the dump is evidence.
    TEST_ASSERT_EQUAL_STRING("", GattNameTable::serviceName(0x1234));
    TEST_ASSERT_EQUAL_STRING("", GattNameTable::characteristicName(0x2AFF));
    TEST_ASSERT_EQUAL_STRING("", GattNameTable::descriptorName(0x2A19));
    // A service UUID looked up as a characteristic, and vice versa, must miss.
    TEST_ASSERT_EQUAL_STRING("", GattNameTable::characteristicName(0x180F));
    TEST_ASSERT_EQUAL_STRING("", GattNameTable::serviceName(0x2A19));
}

void test_the_vendor_range_is_labelled_as_vendor() {
    TEST_ASSERT_TRUE(GattNameTable::isVendorShortUuid(0xFFE0));
    TEST_ASSERT_TRUE(GattNameTable::isVendorShortUuid(0xFFF0));
    TEST_ASSERT_TRUE(GattNameTable::isVendorShortUuid(0xFFFF));
    TEST_ASSERT_FALSE(GattNameTable::isVendorShortUuid(0xFEFF));
    TEST_ASSERT_FALSE(GattNameTable::isVendorShortUuid(0x180F));

    // The names must actually say so, so an operator cannot mistake a community
    // mapping for an assigned one.
    TEST_ASSERT_EQUAL_STRING("Triones / ELK-BLEDOM LED service (vendor)",
                             GattNameTable::serviceName(0xFFF0));
    TEST_ASSERT_EQUAL_STRING("Triones / ELK-BLEDOM LED control (vendor)",
                             GattNameTable::characteristicName(0xFFF3));
}

void test_resolve_dispatches_on_kind_and_normalises_first() {
    TEST_ASSERT_EQUAL_STRING("Battery",
                             GattNameTable::resolve("0000180f-0000-1000-8000-00805f9b34fb",
                                                    GattUuidKind::Service)
                                 .c_str());
    TEST_ASSERT_EQUAL_STRING("Battery Level",
                             GattNameTable::resolve("2A19", GattUuidKind::Characteristic).c_str());
    TEST_ASSERT_EQUAL_STRING("Client Characteristic Configuration",
                             GattNameTable::resolve("2902", GattUuidKind::Descriptor).c_str());

    // Right UUID, wrong kind: no name.
    TEST_ASSERT_EQUAL_STRING("", GattNameTable::resolve("2A19", GattUuidKind::Service).c_str());
    // No name for an off-base UUID, even though kind is valid.
    TEST_ASSERT_EQUAL_STRING(
        "",
        GattNameTable::resolve("12345678-1234-5678-1234-56789abcdef0", GattUuidKind::Service)
            .c_str());
    // The 0xFFFF "no assigned number" filler must stay nameless.
    TEST_ASSERT_EQUAL_STRING("",
                             GattNameTable::resolve("0000ffff-0000-1000-8000-00805f9b34fb",
                                                    GattUuidKind::Characteristic)
                                 .c_str());
}

void test_kind_label_covers_every_kind() {
    TEST_ASSERT_EQUAL_STRING("service", GattNameTable::kindLabel(GattUuidKind::Service));
    TEST_ASSERT_EQUAL_STRING("characteristic",
                             GattNameTable::kindLabel(GattUuidKind::Characteristic));
    TEST_ASSERT_EQUAL_STRING("descriptor", GattNameTable::kindLabel(GattUuidKind::Descriptor));
}

// B4: `include/ble_database.h` carried 73 service rows, 31 of which this table
// did not have. They are now here, which is what lets the legacy copy go. The
// 26 adopted ones carry the SIG name verbatim; the 5 member UUIDs carry a
// provenance tag rather than a name that looks assigned.
void test_the_legacy_service_rows_are_now_covered_here() {
    // Spots from the 0x183x-0x185x block only the legacy table used to have.
    TEST_ASSERT_EQUAL_STRING("Binary Sensor", GattNameTable::serviceName(0x183B));
    TEST_ASSERT_EQUAL_STRING("Emergency Configuration", GattNameTable::serviceName(0x183C));
    TEST_ASSERT_EQUAL_STRING("Audio Input Control", GattNameTable::serviceName(0x1843));
    TEST_ASSERT_EQUAL_STRING("Electronic Shelf Label", GattNameTable::serviceName(0x1857));
    TEST_ASSERT_EQUAL_STRING("Mesh Proxy Solicitation", GattNameTable::serviceName(0x1859));

    // The row the legacy table got wrong: 0x1822 is Pulse Oximeter; HTTP Proxy
    // is 0x1823. Pinned so the wrong mapping cannot come back through the
    // legacy path.
    TEST_ASSERT_EQUAL_STRING("Pulse Oximeter", GattNameTable::serviceName(0x1822));
    TEST_ASSERT_EQUAL_STRING("HTTP Proxy", GattNameTable::serviceName(0x1823));

    // Member UUIDs must say so, so a community mapping is never read as an
    // assigned number.
    TEST_ASSERT_EQUAL_STRING("Blecon service (member)", GattNameTable::serviceName(0xFD0D));
    TEST_ASSERT_EQUAL_STRING("Fast Pair service (member)", GattNameTable::serviceName(0xFE2C));
    TEST_ASSERT_EQUAL_STRING("Eddystone beacon (member)", GattNameTable::serviceName(0xFEAA));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_spelling_of_a_sig_uuid_collapses_to_the_same_short_form);
    RUN_TEST(test_vendor_uuids_on_the_sig_base_collapse_too);
    RUN_TEST(test_a_uuid_off_the_sig_base_is_not_forced_into_a_short_form);
    RUN_TEST(test_malformed_uuids_are_rejected_rather_than_guessed);
    RUN_TEST(test_tables_are_sorted_and_the_counts_are_pinned);
    RUN_TEST(test_the_legacy_service_rows_are_now_covered_here);
    RUN_TEST(test_well_known_names_resolve_and_unknown_stays_empty);
    RUN_TEST(test_the_vendor_range_is_labelled_as_vendor);
    RUN_TEST(test_resolve_dispatches_on_kind_and_normalises_first);
    RUN_TEST(test_kind_label_covers_every_kind);
    return UNITY_END();
}
