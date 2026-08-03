#pragma once

#include <cstdint>

struct MenuRect {
    MenuRect() = default;
    MenuRect(int16_t xValue, int16_t yValue, int16_t widthValue,
             int16_t heightValue)
        : x(xValue), y(yValue), width(widthValue), height(heightValue) {}

    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;

    int16_t right() const { return static_cast<int16_t>(x + width); }
    int16_t bottom() const { return static_cast<int16_t>(y + height); }
    bool empty() const { return width <= 0 || height <= 0; }
    bool contains(int16_t pointX, int16_t pointY) const;
    bool overlaps(const MenuRect& other) const;
};

enum class MenuLayoutKind : uint8_t {
    MainMenu,
    Submenu,
    RotationOptions,
};

class MenuLayout {
public:
    static MenuLayout mainMenu(int16_t screenWidth, int16_t screenHeight,
                               int16_t itemCount);
    static MenuLayout submenu(int16_t screenWidth, int16_t screenHeight,
                              int16_t itemCount);
    static MenuLayout rotationOptions(int16_t screenWidth, int16_t screenHeight,
                                      int16_t itemCount);

    bool valid() const { return valid_; }
    int16_t screenWidth() const { return screenWidth_; }
    int16_t screenHeight() const { return screenHeight_; }
    int16_t contentTop() const { return contentTop_; }
    int16_t contentBottom() const { return contentBottom_; }
    int16_t columns() const { return columns_; }
    int16_t rows() const { return rows_; }
    int16_t itemCount() const { return itemCount_; }

    MenuRect itemRect(int16_t index) const;
    int16_t hitTest(int16_t x, int16_t y) const;

private:
    static MenuLayout create(MenuLayoutKind kind, int16_t screenWidth,
                             int16_t screenHeight, int16_t itemCount);

    int16_t screenWidth_ = 0;
    int16_t screenHeight_ = 0;
    int16_t contentTop_ = 0;
    int16_t contentBottom_ = 0;
    int16_t itemCount_ = 0;
    int16_t columns_ = 0;
    int16_t rows_ = 0;
    int16_t marginX_ = 0;
    int16_t gapX_ = 0;
    int16_t gapY_ = 0;
    int16_t itemWidth_ = 0;
    int16_t itemHeight_ = 0;
    bool valid_ = false;
};
