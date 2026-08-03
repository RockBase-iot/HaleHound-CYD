#pragma once

#include <cstdint>

enum class BoardId : uint8_t {
    Esp32Cyd,
    NmCydC5,
};

struct SpiPins {
    int sck;
    int miso;
    int mosi;
};

struct BoardProfile {
    BoardId id;
    const char* name;
    const char* mcu;
    uint32_t flashBytes;
    bool singleCore;
    uint16_t screenWidth;
    uint16_t screenHeight;
    int backlightPin;
    int touchCsPin;
    SpiPins sharedSpi;
    bool hasOnboardSd;
};

const BoardProfile& getBoardProfile(BoardId id);
const BoardProfile& currentBoardProfile();

namespace board {
using ::BoardProfile;
using ::BoardId;
using ::SpiPins;

extern const BoardProfile CURRENT;
}
