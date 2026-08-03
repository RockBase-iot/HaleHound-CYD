#pragma once

#include <cstdint>
#include <vector>

struct IrFrame {
    uint32_t carrierHz = 0;
    std::vector<uint32_t> pulsesUs;
};

class IrCodec {
public:
    static bool valid(const IrFrame& frame) {
        if (frame.carrierHz < 20000U || frame.carrierHz > 60000U || frame.pulsesUs.empty()) {
            return false;
        }
        for (uint32_t pulse : frame.pulsesUs) {
            if (pulse == 0U || pulse > 200000U) return false;
        }
        return true;
    }

    static std::vector<uint32_t> encode(const IrFrame& frame) {
        return valid(frame) ? frame.pulsesUs : std::vector<uint32_t>{};
    }
};

class IFeature;
IFeature* createIrFeature();
