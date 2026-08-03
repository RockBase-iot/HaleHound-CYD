#include "board/BoardProfile.h"

namespace {
constexpr BoardProfile kEsp32Cyd{
    BoardId::Esp32Cyd,
    "ESP32-2432S028",
    "ESP32-D0WD",
    4U * 1024U * 1024U,
    false,
    240,
    320,
    21,
    33,
    {18, 19, 23},
    false,
};

constexpr BoardProfile kNmCydC5{
    BoardId::NmCydC5,
    "NM-CYD-C5",
    "ESP32-C5",
    16U * 1024U * 1024U,
    true,
    240,
    320,
    25,
    1,
    {6, 2, 7},
    true,
};
}

const BoardProfile& getBoardProfile(BoardId id) {
    return id == BoardId::NmCydC5 ? kNmCydC5 : kEsp32Cyd;
}

const BoardProfile& currentBoardProfile() {
#if defined(NM_CYD_C5) || defined(CYD_C5)
    return kNmCydC5;
#else
    return kEsp32Cyd;
#endif
}

namespace board {
const BoardProfile CURRENT = currentBoardProfile();
}
