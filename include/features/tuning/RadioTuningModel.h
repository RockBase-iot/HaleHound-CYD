#pragma once

// Radio tuning cockpit models.
//
// Upstream v3.9.1 added a dedicated NRF24 tuning panel and v3.9.2 added the
// matching CC1101 SubGHz cockpit. Both share one rule that this module encodes:
// each radio exposes only the controls it can actually honour, every setting is
// applied live, and settings persist. v3.9.5 additionally removed the SubGHz
// bands the 433 MHz front end physically cannot hear (868/915/925) so the list
// stops offering silence that looks like a bug.
//
// Nothing here touches hardware. The models clamp, gate and describe; the
// radio drivers apply.

#include <cstdint>

class IFeature;

// ---------------------------------------------------------------- 2.4 GHz ---

enum class Nrf24Jammer : uint8_t {
    Ble,
    Wlan,
    ProtoKill,
    SubGhz,
};

enum class Nrf24Modulation : uint8_t {
    Cw,    // continuous carrier, the original narrowband spike
    Gfsk,  // modulated noise filling roughly 2 MHz per channel
};

struct Nrf24TuningCapability {
    bool spiClock = false;
    bool power = false;
    bool modulation = false;
    bool dataRate = false;
    bool dwell = false;
    bool gfskPacketLength = false;
    bool bandChannels = false;
};

struct Nrf24TuningSettings {
    uint8_t spiClockMhz = 4;          // 1..10 MHz
    int8_t powerDbm = 0;              // 0, -6, -12, -18
    Nrf24Modulation modulation = Nrf24Modulation::Cw;
    uint16_t dataRateKbps = 2000;     // 250, 1000, 2000
    uint16_t dwellUs = 2000;          // per-channel dwell
    uint8_t gfskPacketLength = 32;    // 1..32 bytes
    uint8_t bandMask = 0x0F;          // which of the four band groups are enabled
};

enum class Nrf24Band : uint8_t {
    BleAdvertising = 0x01,
    Wifi1_6_11 = 0x02,
    Zigbee = 0x04,
    IsmMisc = 0x08,
};

// ---------------------------------------------------------------- SubGHz ----

struct Cc1101TuningSettings {
    // 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK
    uint8_t modulation = 0;
    uint32_t dataRateBaud = 4800;
    uint32_t rxBandwidthHz = 203000;
    uint8_t agcSensitivity = 3;   // driver-dependent gain step
    int32_t frequencyOffsetHz = 0;
};

struct SubGhzBandSpec {
    const char* label;
    uint32_t lowHz;
    uint32_t highHz;
    bool offered;
    const char* note;
};

class RadioTuningModel {
public:
    // ---- 2.4 GHz ----------------------------------------------------------
    static Nrf24TuningCapability capability(Nrf24Jammer jammer);
    static Nrf24TuningSettings defaults(Nrf24Jammer jammer);
    // Gates and clamps ``desired`` for the given jammer. ``droppedControls``
    // counts the knobs the jammer cannot honour, so the UI can hide them
    // instead of drawing dead controls.
    static Nrf24TuningSettings apply(Nrf24Jammer jammer,
                                     const Nrf24TuningSettings& desired,
                                     uint8_t& droppedControls);
    static bool acceptsDataRate(uint16_t kbps);
    // nRF24 channel (0..125) whose centre frequency is ``frequencyMhz``.
    static uint8_t channelForFrequencyMhz(uint16_t frequencyMhz);
    static uint8_t channelsForBand(Nrf24Band band, uint8_t* out, uint8_t capacity);
    static const char* jammerLabel(Nrf24Jammer jammer);

    // ---- SubGHz -----------------------------------------------------------
    static const SubGhzBandSpec* bands();
    static uint8_t bandCount();
    // True when the front end can physically receive this frequency. Bands
    // listed with offered = false are rejected here as well as hidden in the
    // UI, so a stored profile cannot resurrect them.
    static bool frequencySupported(uint32_t frequencyHz);
    static Cc1101TuningSettings defaultsCc1101();
    static Cc1101TuningSettings clampCc1101(const Cc1101TuningSettings& desired);

    // Combined default: the SubGHz cockpit applies the same clamp rules in raw
    // and packet mode, which is why both entry points funnel through here.
    static uint32_t nearestSupportedBand(uint32_t frequencyHz);
};

IFeature* createNrf24TuningFeature();
IFeature* createSubGhzTuningFeature();
