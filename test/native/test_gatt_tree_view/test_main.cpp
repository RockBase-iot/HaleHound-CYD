// The GATT tree as the panel consumes it.
//
// Everything here is about the two ways a flat list on a small screen goes
// wrong: the cursor ends up off screen (the list looks blank), and the cursor
// ends up on something that cannot be acted on (read/write hit nothing). Both
// are properties of this conversion, so both are asserted here instead of being
// discovered on the bench.

#include <unity.h>

#include <cstring>
#include <string>

#include "features/ble/GattTreeView.h"

namespace {

// A peer with three services and a handful of characteristics, shaped like
// something SkeletonKey would actually meet.
SkeletonTree sampleTree() {
    SkeletonTree tree;

    SkeletonService vendor;
    vendor.uuid = "0000FFF0-0000-1000-8000-00805F9B34FB";
    vendor.name = "Triones / ELK-BLEDOM LED service (vendor)";
    vendor.characteristics.push_back(
        SkeletonCharacteristic{"0000FFF3-0000-1000-8000-00805F9B34FB", "", 0x0C, ""});
    vendor.characteristics.push_back(
        SkeletonCharacteristic{"0000FFF4-0000-1000-8000-00805F9B34FB", "", 0x12, "0A0B"});
    tree.services.push_back(vendor);

    SkeletonService battery;
    battery.uuid = "0000180F-0000-1000-8000-00805F9B34FB";
    battery.name = "Battery";
    battery.characteristics.push_back(
        SkeletonCharacteristic{"00002A19-0000-1000-8000-00805F9B34FB", "Battery Level", 0x12,
                               "64"});
    tree.services.push_back(battery);

    // An unknown UUID: no resolved name, so the row falls back to the raw UUID.
    SkeletonService unknown;
    unknown.uuid = "12345678-1234-5678-1234-56789ABCDEF0";
    tree.services.push_back(unknown);

    return tree;
}

}  // namespace

// ── Collapsing and expansion ──────────────────────────────────────────────

void test_a_fresh_tree_starts_collapsed_with_the_cursor_on_the_first_service() {
    GattTreeView view;
    view.setTree(sampleTree());

    TEST_ASSERT_FALSE(view.empty());
    TEST_ASSERT_EQUAL_UINT16(3, view.rowCount());
    TEST_ASSERT_EQUAL_UINT16(0, view.selectedRow());
    TEST_ASSERT_FALSE(view.expanded(0));

    const std::vector<GattTreeRow> rows = view.rows();
    TEST_ASSERT_EQUAL(3, static_cast<int>(rows.size()));
    for (const GattTreeRow& row : rows) {
        TEST_ASSERT_TRUE(row.isService());
        TEST_ASSERT_EQUAL_UINT8(0, row.depth);
    }
    TEST_ASSERT_TRUE(rows[0].hasChildren);
    TEST_ASSERT_TRUE(rows[0].expanded == false);
    TEST_ASSERT_FALSE(rows[2].hasChildren);
}

void test_expanding_puts_the_characteristics_directly_under_their_service() {
    GattTreeView view;
    view.setTree(sampleTree());
    view.expand(0);

    TEST_ASSERT_EQUAL_UINT16(5, view.rowCount());
    const std::vector<GattTreeRow> rows = view.rows();
    // Service, its two characteristics, then the two remaining services.
    TEST_ASSERT_TRUE(rows[0].isService());
    TEST_ASSERT_EQUAL_STRING("0000FFF0-0000-1000-8000-00805F9B34FB", rows[0].uuid.c_str());
    TEST_ASSERT_EQUAL_UINT8(1, rows[1].depth);
    TEST_ASSERT_EQUAL_UINT8(1, rows[2].depth);
    TEST_ASSERT_EQUAL_UINT16(0, rows[1].serviceIndex);
    TEST_ASSERT_EQUAL_UINT16(0, rows[1].characteristicIndex);
    TEST_ASSERT_EQUAL_UINT16(1, rows[2].characteristicIndex);
    TEST_ASSERT_TRUE(rows[3].isService());

    // A characteristic carries the property letters and its last value, which is
    // what the detail line and the hex panel print.
    TEST_ASSERT_EQUAL_STRING("w W", rows[1].detail.c_str());
    TEST_ASSERT_EQUAL_STRING("", rows[1].value.c_str());
    TEST_ASSERT_EQUAL_STRING("R N", rows[2].detail.c_str());
    TEST_ASSERT_EQUAL_STRING("0A0B", rows[2].value.c_str());

    // Collapsing takes them away again.
    view.collapse(0);
    TEST_ASSERT_EQUAL_UINT16(3, view.rowCount());
}

void test_toggle_and_expand_all_agree_on_the_row_set() {
    GattTreeView view;
    view.setTree(sampleTree());

    view.toggle(0);
    view.toggle(1);
    TEST_ASSERT_TRUE(view.expanded(0));
    TEST_ASSERT_TRUE(view.expanded(1));
    TEST_ASSERT_FALSE(view.expanded(2));
    // 3 services + 2 + 1 characteristics.
    TEST_ASSERT_EQUAL_UINT16(6, view.rowCount());

    view.toggle(0);
    TEST_ASSERT_FALSE(view.expanded(0));
    // Collapsing service 0 takes its two characteristics with it: 6 - 2.
    TEST_ASSERT_EQUAL_UINT16(4, view.rowCount());

    view.expandAll();
    TEST_ASSERT_EQUAL_UINT16(6, view.rowCount());
    view.collapseAll();
    TEST_ASSERT_EQUAL_UINT16(3, view.rowCount());

    // Out of range is ignored, not wrapped onto a neighbouring service.
    view.toggle(9);
    TEST_ASSERT_FALSE(view.expanded(9));
}

void test_a_row_with_no_resolved_name_prints_its_raw_uuid() {
    GattTreeView view;
    view.setTree(sampleTree());

    const std::vector<GattTreeRow> rows = view.rows();
    TEST_ASSERT_EQUAL_STRING("Battery", rows[1].label().c_str());
    // Unknown UUID: the label is the UUID itself. No invented name.
    TEST_ASSERT_EQUAL_STRING("12345678-1234-5678-1234-56789ABCDEF0", rows[2].label().c_str());
    TEST_ASSERT_EQUAL_STRING("", rows[2].name.c_str());
}

// ── The cursor is always on screen ────────────────────────────────────────

void test_scrolling_pulls_the_page_along_so_the_cursor_is_never_off_screen() {
    SkeletonTree tree;
    for (int index = 0; index < 20; ++index) {
        SkeletonService service;
        service.uuid = std::string("0000180A-0000-1000-8000-00805F9B34FB");
        service.name = "service";
        tree.services.push_back(service);
    }
    GattTreeView view;
    view.setTree(tree);
    view.setPageSize(4);

    TEST_ASSERT_EQUAL_UINT16(5, view.pageCount());
    TEST_ASSERT_EQUAL_UINT16(0, view.page());

    // Walk the cursor down the list one row at a time; after every step the
    // selected row must be inside the visible window.
    for (uint16_t expected = 0; expected < 20; ++expected) {
        TEST_ASSERT_EQUAL_UINT16(expected, view.selectedRow());
        const std::uint32_t firstVisible =
            static_cast<std::uint32_t>(view.page()) * view.pageSize();
        const std::uint32_t lastVisible = firstVisible + view.pageSize() - 1U;
        TEST_ASSERT_TRUE(view.selectedRow() >= firstVisible);
        TEST_ASSERT_TRUE(view.selectedRow() <= lastVisible);
        view.scrollBy(1);
    }
    // Scrolling past the end clamps to the last row instead of running off.
    TEST_ASSERT_EQUAL_UINT16(19, view.selectedRow());
    view.scrollBy(50);
    TEST_ASSERT_EQUAL_UINT16(19, view.selectedRow());
    view.scrollBy(-100);
    TEST_ASSERT_EQUAL_UINT16(0, view.selectedRow());
    TEST_ASSERT_EQUAL_UINT16(0, view.page());
}

void test_visible_rows_are_the_current_page_and_nothing_else() {
    SkeletonTree tree;
    for (int index = 0; index < 10; ++index) {
        SkeletonService service;
        service.uuid = "0000180A-0000-1000-8000-00805F9B34FB";
        service.name = "s";
        tree.services.push_back(service);
    }
    GattTreeView view;
    view.setTree(tree);
    view.setPageSize(4);

    TEST_ASSERT_EQUAL(4, static_cast<int>(view.visibleRows().size()));
    view.goToPage(1);
    TEST_ASSERT_EQUAL(4, static_cast<int>(view.visibleRows().size()));
    // The last page is a partial one, and must not read past the end.
    view.goToPage(2);
    TEST_ASSERT_EQUAL(2, static_cast<int>(view.visibleRows().size()));
    // Out of range lands on the last page rather than on an empty one.
    view.goToPage(99);
    TEST_ASSERT_EQUAL_UINT16(2, view.page());
    TEST_ASSERT_EQUAL(2, static_cast<int>(view.visibleRows().size()));
}

void test_a_page_jump_moves_the_cursor_onto_that_page() {
    SkeletonTree tree;
    for (int index = 0; index < 12; ++index) {
        SkeletonService service;
        service.uuid = "0000180A-0000-1000-8000-00805F9B34FB";
        service.name = "s";
        tree.services.push_back(service);
    }
    GattTreeView view;
    view.setTree(tree);
    view.setPageSize(5);

    view.goToPage(2);
    TEST_ASSERT_EQUAL_UINT16(10, view.selectedRow());
    TEST_ASSERT_EQUAL_UINT16(2, view.page());
}

void test_collapsing_under_the_cursor_clamps_it_instead_of_leaving_it_dangling() {
    GattTreeView view;
    view.setTree(sampleTree());
    view.expandAll();
    TEST_ASSERT_EQUAL_UINT16(6, view.rowCount());

    // Put the cursor on the last row (a characteristic of a later service), then
    // collapse everything: the row list shrinks under it.
    view.selectRow(5);
    TEST_ASSERT_EQUAL_UINT16(5, view.selectedRow());
    view.collapseAll();
    TEST_ASSERT_EQUAL_UINT16(3, view.rowCount());
    TEST_ASSERT_EQUAL_UINT16(2, view.selectedRow());

    // And with the list emptied entirely, there is no selection at all.
    view.setTree(SkeletonTree{});
    TEST_ASSERT_EQUAL_UINT16(GattTreeView::kNoSelection, view.selectedRow());
}

// ── Selection only lands on something actionable ──────────────────────────

void test_only_a_characteristic_row_can_be_addressed() {
    GattTreeView view;
    view.setTree(sampleTree());
    view.expand(0);

    uint16_t serviceIndex = 0;
    uint16_t characteristicIndex = 0;

    // Row 0 is the service: selecting it is fine (that is what expand acts on),
    // but it is not something to read or write.
    view.selectRow(0);
    TEST_ASSERT_FALSE(view.selectedCharacteristic(serviceIndex, characteristicIndex));

    // Row 1 is the first characteristic.
    view.selectRow(1);
    TEST_ASSERT_TRUE(view.selectedCharacteristic(serviceIndex, characteristicIndex));
    TEST_ASSERT_EQUAL_UINT16(0, serviceIndex);
    TEST_ASSERT_EQUAL_UINT16(0, characteristicIndex);

    // Row 2 is the second.
    view.selectRow(2);
    TEST_ASSERT_TRUE(view.selectedCharacteristic(serviceIndex, characteristicIndex));
    TEST_ASSERT_EQUAL_UINT16(1, characteristicIndex);

    // An empty tree has nothing to address, so a read button cannot fire.
    view.setTree(SkeletonTree{});
    TEST_ASSERT_FALSE(view.selectedCharacteristic(serviceIndex, characteristicIndex));
    TEST_ASSERT_EQUAL_UINT16(0, view.rowCount());
    TEST_ASSERT_EQUAL(0, static_cast<int>(view.visibleRows().size()));
    TEST_ASSERT_EQUAL_UINT16(0, view.pageCount());
    TEST_ASSERT_TRUE(view.empty());
}

void test_a_truncated_service_is_marked_on_its_row() {
    SkeletonTree tree = sampleTree();
    tree.services[0].truncated = true;
    tree.truncated = true;

    GattTreeView view;
    view.setTree(tree);
    TEST_ASSERT_TRUE(view.truncated());
    TEST_ASSERT_TRUE(view.rows()[0].truncated);
    TEST_ASSERT_FALSE(view.rows()[1].truncated);
}

// ── Property letters ─────────────────────────────────────────────────────

void test_property_letters_follow_the_declaration_order() {
    char label[16] = {};

    TEST_ASSERT_EQUAL(0, static_cast<int>(GattTreeView::propertyLabel(0x00, label, sizeof(label))));
    TEST_ASSERT_EQUAL_STRING("", label);

    TEST_ASSERT_EQUAL(1, static_cast<int>(GattTreeView::propertyLabel(0x02, label, sizeof(label))));
    TEST_ASSERT_EQUAL_STRING("R", label);

    GattTreeView::propertyLabel(0x1E, label, sizeof(label));  // read|wwr|write|notify
    TEST_ASSERT_EQUAL_STRING("R w W N", label);

    // Every bit set, in order.
    GattTreeView::propertyLabel(0x3F, label, sizeof(label));
    TEST_ASSERT_EQUAL_STRING("B R w W N I", label);

    // A buffer with no room refuses rather than writing an unterminated label.
    char tiny[4] = {};
    const std::size_t written = GattTreeView::propertyLabel(0x3F, tiny, sizeof(tiny));
    TEST_ASSERT_TRUE(written <= sizeof(tiny) - 1U);
    TEST_ASSERT_EQUAL_CHAR('\0', tiny[written]);
    TEST_ASSERT_EQUAL(0, static_cast<int>(GattTreeView::propertyLabel(0x3F, nullptr, 16)));
    TEST_ASSERT_EQUAL(0, static_cast<int>(GattTreeView::propertyLabel(0x3F, label, 0)));
}

void test_a_page_size_of_zero_is_treated_as_one_not_as_a_divide_by_zero() {
    GattTreeView view;
    view.setTree(sampleTree());
    view.setPageSize(0);

    TEST_ASSERT_EQUAL_UINT16(1, view.pageSize());
    TEST_ASSERT_EQUAL_UINT16(3, view.pageCount());
    TEST_ASSERT_EQUAL(1, static_cast<int>(view.visibleRows().size()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_a_fresh_tree_starts_collapsed_with_the_cursor_on_the_first_service);
    RUN_TEST(test_expanding_puts_the_characteristics_directly_under_their_service);
    RUN_TEST(test_toggle_and_expand_all_agree_on_the_row_set);
    RUN_TEST(test_a_row_with_no_resolved_name_prints_its_raw_uuid);
    RUN_TEST(test_scrolling_pulls_the_page_along_so_the_cursor_is_never_off_screen);
    RUN_TEST(test_visible_rows_are_the_current_page_and_nothing_else);
    RUN_TEST(test_a_page_jump_moves_the_cursor_onto_that_page);
    RUN_TEST(test_collapsing_under_the_cursor_clamps_it_instead_of_leaving_it_dangling);
    RUN_TEST(test_only_a_characteristic_row_can_be_addressed);
    RUN_TEST(test_a_truncated_service_is_marked_on_its_row);
    RUN_TEST(test_property_letters_follow_the_declaration_order);
    RUN_TEST(test_a_page_size_of_zero_is_treated_as_one_not_as_a_divide_by_zero);
    return UNITY_END();
}
