#pragma once

#include <cstdint>

// The drawing surface a feature paints into.
//
// Deliberately primitive and Arduino-free. The core must not know about
// TFT_eSPI, and the legacy screens keep driving it directly -- this seam exists
// only so a feature that owns a screen of its own (the GATT tree, the Chameleon
// slot table) has somewhere to draw that is not the legacy monolith.
//
// `IFeature::draw()` returns whether the feature painted. When it did, the
// runner reports `ownsScreen()` and the caller must not paint its own chrome
// over the frame. That is the whole contract; everything else is pixels.
//
// Colours are RGB565, the same encoding the panel takes, so an adapter is a
// straight pass-through and a host test can assert exact values.
//
// Coordinates are device pixels with the origin at the top-left of the usable
// area. A canvas exposes its own size rather than the feature assuming one: the
// repository has 2.8" and 3.5" boards, and a feature that hard-codes 320x240 is
// a feature that is wrong on one of them.
class IFeatureCanvas {
public:
    virtual ~IFeatureCanvas() = default;

    virtual int16_t width() const = 0;
    virtual int16_t height() const = 0;

    // Height of one line of `drawText` at size 1, so a feature can lay out rows
    // without hard-coding the font metrics of whichever backend is behind this.
    virtual int16_t lineHeight() const = 0;

    virtual void clear(uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t width, int16_t height,
                          uint16_t color) = 0;
    // 1px outline. `drawRect(x, y, w, h)` frames the rectangle itself: the
    // stroke is inside [x, x + w] and [y, y + h], so a caller can frame the
    // whole screen with drawRect(0, 0, width() - 1, height() - 1) without
    // deciding the off-by-one differently from the backend.
    virtual void drawRect(int16_t x, int16_t y, int16_t width, int16_t height,
                          uint16_t color) = 0;

    virtual void drawText(int16_t x, int16_t y, const char* text, uint16_t color,
                          uint8_t size) = 0;
    // Draws only what fits in `maxWidth`, ellipsing the remainder. A GATT name
    // that does not fit a row must be visibly cut, not silently run over the
    // next column -- and not silently dropped either.
    virtual void drawTextClipped(int16_t x, int16_t y, int16_t maxWidth,
                                 const char* text, uint16_t color, uint8_t size) = 0;

    // The frame is complete. A direct-to-panel backend does nothing here; a
    // sprite-backed one pushes the buffer. Explicit because "when is it safe to
    // see the frame" is a backend decision, and inferring it from the last draw
    // call makes a double-buffered adapter impossible to write.
    virtual void present() = 0;
};
