#include <unity.h>

#include "services/MenuLayout.h"

namespace {

void assertLayoutGeometry(const MenuLayout& layout, int expectedColumns,
                          int expectedRows, int itemCount) {
    TEST_ASSERT_TRUE(layout.valid());
    TEST_ASSERT_EQUAL(expectedColumns, layout.columns());
    TEST_ASSERT_EQUAL(expectedRows, layout.rows());
    TEST_ASSERT_EQUAL(itemCount, layout.itemCount());

    for (int i = 0; i < itemCount; ++i) {
        const MenuRect rect = layout.itemRect(i);
        TEST_ASSERT_FALSE(rect.empty());
        TEST_ASSERT_GREATER_OR_EQUAL(0, rect.x);
        TEST_ASSERT_GREATER_OR_EQUAL(layout.contentTop(), rect.y);
        TEST_ASSERT_LESS_OR_EQUAL(layout.screenWidth(), rect.right());
        TEST_ASSERT_LESS_OR_EQUAL(layout.contentBottom(), rect.bottom());
        TEST_ASSERT_EQUAL(i, layout.hitTest(rect.x + rect.width / 2,
                                           rect.y + rect.height / 2));

        for (int j = i + 1; j < itemCount; ++j) {
            TEST_ASSERT_FALSE(rect.overlaps(layout.itemRect(j)));
        }
    }
}

void test_main_menu_fits_portrait_and_landscape() {
    assertLayoutGeometry(MenuLayout::mainMenu(240, 320, 10), 2, 5, 10);
    assertLayoutGeometry(MenuLayout::mainMenu(320, 240, 10), 2, 5, 10);
}

void test_submenu_changes_from_one_column_to_two_columns() {
    assertLayoutGeometry(MenuLayout::submenu(240, 320, 10), 1, 10, 10);
    assertLayoutGeometry(MenuLayout::submenu(320, 240, 10), 2, 5, 10);
    assertLayoutGeometry(MenuLayout::submenu(320, 240, 9), 2, 5, 9);
    assertLayoutGeometry(MenuLayout::submenu(240, 320, 1), 1, 1, 1);
}

void test_landscape_submenu_uses_column_major_order() {
    const MenuLayout layout = MenuLayout::submenu(320, 240, 9);
    const MenuRect first = layout.itemRect(0);
    const MenuRect fifth = layout.itemRect(4);
    const MenuRect sixth = layout.itemRect(5);

    TEST_ASSERT_EQUAL(first.x, fifth.x);
    TEST_ASSERT_GREATER_THAN(first.y, fifth.y);
    TEST_ASSERT_GREATER_THAN(first.x, sixth.x);
    TEST_ASSERT_EQUAL(first.y, sixth.y);
}

void test_rotation_options_use_two_by_two_landscape_grid() {
    const MenuLayout portrait = MenuLayout::rotationOptions(240, 320, 4);
    const MenuLayout landscape = MenuLayout::rotationOptions(320, 240, 4);

    assertLayoutGeometry(portrait, 1, 4, 4);
    assertLayoutGeometry(landscape, 2, 2, 4);
    TEST_ASSERT_LESS_OR_EQUAL(260, portrait.contentBottom());
    TEST_ASSERT_LESS_OR_EQUAL(212, landscape.contentBottom());
}

void test_invalid_indexes_and_gaps_do_not_hit() {
    const MenuLayout layout = MenuLayout::submenu(320, 240, 10);
    TEST_ASSERT_TRUE(layout.itemRect(-1).empty());
    TEST_ASSERT_TRUE(layout.itemRect(10).empty());
    TEST_ASSERT_EQUAL(-1, layout.hitTest(0, 0));

    const MenuRect first = layout.itemRect(0);
    TEST_ASSERT_EQUAL(-1, layout.hitTest(first.right(), first.y));
    TEST_ASSERT_EQUAL(-1, layout.hitTest(first.x, first.bottom()));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_main_menu_fits_portrait_and_landscape);
    RUN_TEST(test_submenu_changes_from_one_column_to_two_columns);
    RUN_TEST(test_landscape_submenu_uses_column_major_order);
    RUN_TEST(test_rotation_options_use_two_by_two_landscape_grid);
    RUN_TEST(test_invalid_indexes_and_gaps_do_not_hit);
    return UNITY_END();
}
