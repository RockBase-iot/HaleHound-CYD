#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "core/IFeatureCanvas.h"

// The host test double for IFeatureCanvas -- the same role FakeBleCentral plays
// for IBleCentral. It records what was asked for instead of drawing it, so a
// feature's output can be asserted rather than eyeballed on a panel.
//
// It answers width/height/lineHeight from values the test sets, on purpose: a
// feature that hard-codes 320x240 is wrong on one of the two supported boards,
// and a double that also hard-coded it would hide exactly that.
//
// Text is copied into a fixed buffer rather than borrowed, so a test can hold a
// call after the feature's own storage is gone.
class FakeFeatureCanvas final : public IFeatureCanvas {
public:
    static constexpr size_t kMaxTexts = 32;
    static constexpr size_t kMaxRects = 32;
    static constexpr size_t kMaxTextLength = 48;

    struct TextCall {
        int16_t x = 0;
        int16_t y = 0;
        // -1 for an unclipped drawText, otherwise the width drawTextClipped was
        // given. Distinguishing the two is the point: a row that was supposed to
        // be clipped and was not is a layout bug that still renders.
        int16_t maxWidth = -1;
        char text[kMaxTextLength] = {};
        uint16_t color = 0;
        uint8_t size = 0;
    };

    struct RectCall {
        int16_t x = 0;
        int16_t y = 0;
        int16_t width = 0;
        int16_t height = 0;
        uint16_t color = 0;
        bool filled = false;
    };

    explicit FakeFeatureCanvas(int16_t canvasWidth = 240, int16_t canvasHeight = 320,
                               int16_t canvasLineHeight = 8)
        : panelWidth(canvasWidth), panelHeight(canvasHeight),
          panelLineHeight(canvasLineHeight) {}

    int16_t width() const override { return panelWidth; }
    int16_t height() const override { return panelHeight; }
    int16_t lineHeight() const override { return panelLineHeight; }

    void clear(uint16_t color) override {
        ++clearCount;
        lastClearColor = color;
    }

    void fillRect(int16_t x, int16_t y, int16_t width, int16_t height,
                  uint16_t color) override {
        recordRect(x, y, width, height, color, true);
    }

    void drawRect(int16_t x, int16_t y, int16_t width, int16_t height,
                  uint16_t color) override {
        recordRect(x, y, width, height, color, false);
    }

    void drawText(int16_t x, int16_t y, const char* text, uint16_t color,
                  uint8_t size) override {
        recordText(x, y, -1, text, color, size);
    }

    void drawTextClipped(int16_t x, int16_t y, int16_t maxWidth, const char* text,
                         uint16_t color, uint8_t size) override {
        recordText(x, y, maxWidth, text, color, size);
    }

    void present() override { ++presentCount; }

    // ─── What the test asserts on ───────────────────────────────────────────
    size_t textCount = 0;
    TextCall texts[kMaxTexts]{};
    size_t rectCount = 0;
    RectCall rects[kMaxRects]{};
    size_t clearCount = 0;
    uint16_t lastClearColor = 0;
    size_t presentCount = 0;

    // Overflows rather than silently dropping: a feature that painted more than
    // the double can hold has a test that would otherwise under-report.
    size_t textOverflow = 0;
    size_t rectOverflow = 0;

    // The text of the nth drawText/drawTextClipped call, or "" past the end.
    const char* textAt(size_t index) const {
        return index < textCount ? texts[index].text : "";
    }

    bool anyTextContains(const char* needle) const {
        for (size_t index = 0; index < textCount; ++index) {
            if (std::strstr(texts[index].text, needle) != nullptr) {
                return true;
            }
        }
        return false;
    }

    void reset() {
        textCount = 0;
        rectCount = 0;
        clearCount = 0;
        lastClearColor = 0;
        presentCount = 0;
        textOverflow = 0;
        rectOverflow = 0;
    }

    // The test controls these to model a board.
    int16_t panelWidth;
    int16_t panelHeight;
    int16_t panelLineHeight;

private:
    void recordText(int16_t x, int16_t y, int16_t maxWidth, const char* text,
                    uint16_t color, uint8_t size) {
        if (textCount >= kMaxTexts) {
            ++textOverflow;
            return;
        }
        TextCall& call = texts[textCount++];
        call.x = x;
        call.y = y;
        call.maxWidth = maxWidth;
        call.color = color;
        call.size = size;
        if (text != nullptr) {
            std::strncpy(call.text, text, kMaxTextLength - 1);
            call.text[kMaxTextLength - 1] = '\0';
        }
    }

    void recordRect(int16_t x, int16_t y, int16_t width, int16_t height,
                    uint16_t color, bool filled) {
        if (rectCount >= kMaxRects) {
            ++rectOverflow;
            return;
        }
        RectCall& call = rects[rectCount++];
        call.x = x;
        call.y = y;
        call.width = width;
        call.height = height;
        call.color = color;
        call.filled = filled;
    }
};
