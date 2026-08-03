#include "services/MenuLayout.h"

bool MenuRect::contains(int16_t pointX, int16_t pointY) const {
    return !empty() && pointX >= x && pointX < right() &&
           pointY >= y && pointY < bottom();
}

bool MenuRect::overlaps(const MenuRect& other) const {
    if (empty() || other.empty()) return false;
    return x < other.right() && right() > other.x &&
           y < other.bottom() && bottom() > other.y;
}

MenuLayout MenuLayout::mainMenu(int16_t screenWidth, int16_t screenHeight,
                                int16_t itemCount) {
    return create(MenuLayoutKind::MainMenu, screenWidth, screenHeight, itemCount);
}

MenuLayout MenuLayout::submenu(int16_t screenWidth, int16_t screenHeight,
                               int16_t itemCount) {
    return create(MenuLayoutKind::Submenu, screenWidth, screenHeight, itemCount);
}

MenuLayout MenuLayout::rotationOptions(int16_t screenWidth, int16_t screenHeight,
                                       int16_t itemCount) {
    return create(MenuLayoutKind::RotationOptions, screenWidth, screenHeight,
                  itemCount);
}

MenuLayout MenuLayout::create(MenuLayoutKind kind, int16_t screenWidth,
                              int16_t screenHeight, int16_t itemCount) {
    MenuLayout layout;
    layout.screenWidth_ = screenWidth;
    layout.screenHeight_ = screenHeight;
    layout.itemCount_ = itemCount;

    if (screenWidth <= 0 || screenHeight <= 0 || itemCount <= 0) return layout;

    const bool landscape = screenWidth > screenHeight;
    switch (kind) {
        case MenuLayoutKind::MainMenu:
            layout.contentTop_ = 26;
            layout.contentBottom_ = static_cast<int16_t>(screenHeight - 24);
            layout.columns_ = itemCount > 1 ? 2 : 1;
            layout.marginX_ = 10;
            layout.gapX_ = 12;
            layout.gapY_ = 2;
            break;
        case MenuLayoutKind::Submenu:
            layout.contentTop_ = 28;
            layout.contentBottom_ = static_cast<int16_t>(screenHeight - 6);
            layout.columns_ = landscape && itemCount > 1 ? 2 : 1;
            layout.marginX_ = 8;
            layout.gapX_ = 8;
            layout.gapY_ = 3;
            break;
        case MenuLayoutKind::RotationOptions:
            layout.contentTop_ = landscape ? 82 : 88;
            layout.contentBottom_ = static_cast<int16_t>(
                screenHeight - (landscape ? 28 : 60));
            layout.columns_ = landscape && itemCount > 1 ? 2 : 1;
            layout.marginX_ = 20;
            layout.gapX_ = 8;
            layout.gapY_ = 6;
            break;
    }

    layout.rows_ = static_cast<int16_t>((itemCount + layout.columns_ - 1) /
                                        layout.columns_);
    const int16_t horizontalGaps = static_cast<int16_t>(
        layout.gapX_ * (layout.columns_ - 1));
    const int16_t verticalGaps = static_cast<int16_t>(
        layout.gapY_ * (layout.rows_ - 1));
    const int16_t usableWidth = static_cast<int16_t>(
        screenWidth - 2 * layout.marginX_ - horizontalGaps);
    const int16_t usableHeight = static_cast<int16_t>(
        layout.contentBottom_ - layout.contentTop_ - verticalGaps);

    if (usableWidth < layout.columns_ || usableHeight < layout.rows_) {
        return layout;
    }

    layout.itemWidth_ = static_cast<int16_t>(usableWidth / layout.columns_);
    layout.itemHeight_ = static_cast<int16_t>(usableHeight / layout.rows_);
    layout.valid_ = layout.itemWidth_ > 0 && layout.itemHeight_ > 0;
    return layout;
}

MenuRect MenuLayout::itemRect(int16_t index) const {
    if (!valid_ || index < 0 || index >= itemCount_) return {};

    // Column-major order preserves the legacy top-to-bottom menu sequence.
    const int16_t column = static_cast<int16_t>(index / rows_);
    const int16_t row = static_cast<int16_t>(index % rows_);
    return {
        static_cast<int16_t>(marginX_ + column * (itemWidth_ + gapX_)),
        static_cast<int16_t>(contentTop_ + row * (itemHeight_ + gapY_)),
        itemWidth_,
        itemHeight_,
    };
}

int16_t MenuLayout::hitTest(int16_t x, int16_t y) const {
    if (!valid_) return -1;
    for (int16_t index = 0; index < itemCount_; ++index) {
        if (itemRect(index).contains(x, y)) return index;
    }
    return -1;
}
