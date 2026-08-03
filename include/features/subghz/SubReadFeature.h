#pragma once

#include <cstdint>

class IFeature;

enum class TeslaChargeMode : uint8_t {
    Us,
    Eu,
    Both,
};

struct TeslaChargeEvidence {
    TeslaChargeMode mode;
    bool supported;
    const char* detail;
};

TeslaChargeEvidence teslaChargeEvidence(TeslaChargeMode mode);

IFeature* createSubReadFeature();
IFeature* createTeslaChargeFeature();
