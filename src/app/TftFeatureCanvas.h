#pragma once

// The TFT_eSPI side of IFeatureCanvas. Arduino-only by construction: it is the
// only file in the drawing path that knows about the panel, and it is a straight
// pass-through -- all the layout logic lives in the feature.
//
// `present()` is a no-op because TFT_eSPI draws straight to the panel. It is
// still called, so a sprite-backed adapter can be dropped in later without any
// feature learning about it.

#include <TFT_eSPI.h>

#include <cstddef>
#include <cstring>

#include "core/IFeatureCanvas.h"

class TftFeatureCanvas final : public IFeatureCanvas {
public:
    explicit TftFeatureCanvas(TFT_eSPI& panel) : panel_(panel) {}

    int16_t width() const override { return static_cast<int16_t>(panel_.width()); }
    int16_t height() const override { return static_cast<int16_t>(panel_.height()); }
    int16_t lineHeight() const override {
        return static_cast<int16_t>(panel_.fontHeight());
    }

    void clear(uint16_t color) override { panel_.fillScreen(color); }

    void fillRect(int16_t x, int16_t y, int16_t width, int16_t height,
                  uint16_t color) override {
        panel_.fillRect(x, y, width, height, color);
    }

    void drawRect(int16_t x, int16_t y, int16_t width, int16_t height,
                  uint16_t color) override {
        panel_.drawRect(x, y, width, height, color);
    }

    void drawText(int16_t x, int16_t y, const char* text, uint16_t color,
                  uint8_t size) override {
        if (text == nullptr) {
            return;
        }
        panel_.setTextSize(size);
        panel_.setTextColor(color);
        panel_.setCursor(x, y);
        panel_.print(text);
    }

    void drawTextClipped(int16_t x, int16_t y, int16_t maxWidth, const char* text,
                         uint16_t color, uint8_t size) override {
        if (text == nullptr || maxWidth <= 0) {
            return;
        }
        panel_.setTextSize(size);
        // textWidth() honours the current text size, so the measurement and the
        // draw below agree.
        if (panel_.textWidth(text) <= maxWidth) {
            drawText(x, y, text, color, size);
            return;
        }

        // TFT_eSPI has no clipped draw. Letting the text run on would overwrite
        // the next column, and silently drawing a bare prefix would present a
        // cut UUID as a whole one -- the exact confusion the seam's UUID type
        // exists to prevent. So: longest fitting prefix, plus a visible marker.
        char clipped[kClipBuffer];
        size_t length = std::strlen(text);
        if (length > kClipBuffer - 2) {
            length = kClipBuffer - 2;
        }
        for (size_t count = length; count > 0; --count) {
            std::memcpy(clipped, text, count);
            clipped[count] = '~';
            clipped[count + 1] = '\0';
            if (panel_.textWidth(clipped) <= maxWidth) {
                drawText(x, y, clipped, color, size);
                return;
            }
        }
        // Not even one character plus the marker fits. A column that narrow is a
        // layout bug, and the marker alone is the most honest thing left.
        drawText(x, y, "~", color, size);
    }

    void present() override {}

private:
    static constexpr size_t kClipBuffer = 64;

    TFT_eSPI& panel_;
};
