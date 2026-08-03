#pragma once

#include <cstdint>

struct Rf433Signal {
    uint32_t frequencyHz = 0;
    uint64_t code = 0;
    uint8_t bits = 0;
    uint8_t protocol = 0;
    bool transmitAuthorized = false;
};

class Rf433Codec {
public:
    static bool valid(const Rf433Signal& signal) {
        return signal.frequencyHz >= 300000000U && signal.frequencyHz <= 928000000U &&
               signal.bits > 0U && signal.bits <= 64U && signal.protocol > 0U;
    }
};

class IFeature;
IFeature* createRf433Feature();
