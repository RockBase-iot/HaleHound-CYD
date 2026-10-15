#include "features/tuning/RadioTuningModel.h"

#include "core/FeatureRunner.h"

namespace {

// The CC1101 front end on these units is matched for 433 MHz. The upper ISM
// entries were removed from the UI in v3.9.5 because the radio is deaf there;
// keeping them in the list produced silence that read as a firmware bug.
const SubGhzBandSpec kSubGhzBands[] = {
    {"315", 307000000UL, 323000000UL, true, "US car and garage fobs; off-match but hears a strong close fob"},
    {"433.92", 426000000UL, 442000000UL, true, "default; the radio's matched home"},
    {"WIDE", 300000000UL, 464000000UL, true, "whole usable range in one sweep"},
    {"868", 863000000UL, 870000000UL, false, "front end cannot receive here"},
    {"915", 902000000UL, 928000000UL, false, "front end cannot receive here"},
    {"925", 925000000UL, 925000000UL, false, "front end cannot receive here"},
};

constexpr Nrf24TuningCapability kFullCapability = {
    true,  // spiClock
    true,  // power
    true,  // modulation
    true,  // dataRate
    true,  // dwell
    true,  // gfskPacketLength
    true,  // bandChannels
};

uint8_t clampByte(uint8_t value, uint8_t low, uint8_t high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

int8_t nearestPowerStep(int8_t requested) {
    // The nRF24 power amplifier has exactly four settings.
    const int8_t steps[4] = {0, -6, -12, -18};
    int8_t best = steps[0];
    int bestDistance = 1000;
    for (int index = 0; index < 4; ++index) {
        const int distance = requested > steps[index] ? requested - steps[index] : steps[index] - requested;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = steps[index];
        }
    }
    return best;
}

// Tuning capabilities per jammer. The list is intentionally explicit: the point
// of the v3.9.1 panel is that a jammer shows only the knobs it can use.
Nrf24TuningCapability capabilityFor(Nrf24Jammer jammer) {
    Nrf24TuningCapability capability = kFullCapability;
    switch (jammer) {
        case Nrf24Jammer::Ble:
            capability.dataRate = true;
            break;
        case Nrf24Jammer::Wlan:
            // WLAN uses the tight 1/6/11 pattern with tuned per-channel dwell,
            // so it does not expose free per-band channel selection.
            capability.bandChannels = false;
            break;
        case Nrf24Jammer::ProtoKill:
            capability.gfskPacketLength = false;
            break;
        case Nrf24Jammer::SubGhz:
            // The SubGHz jammer drives the CC1101, not the nRF24, so it exposes
            // the SubGHz cockpit instead of the 2.4 GHz knobs.
            capability.spiClock = false;
            capability.dataRate = false;
            capability.gfskPacketLength = false;
            capability.bandChannels = false;
            break;
    }
    return capability;
}

}  // namespace

Nrf24TuningCapability RadioTuningModel::capability(Nrf24Jammer jammer) {
    return capabilityFor(jammer);
}

Nrf24TuningSettings RadioTuningModel::defaults(Nrf24Jammer jammer) {
    Nrf24TuningSettings settings;
    switch (jammer) {
        case Nrf24Jammer::Ble:
            // Ble Jammer defaults to the ADV KILL dwell.
            settings.dwellUs = 1500;
            settings.bandMask = 0x01;
            break;
        case Nrf24Jammer::Wlan:
            settings.bandMask = 0x02;
            settings.dwellUs = 4000;
            break;
        case Nrf24Jammer::ProtoKill:
            settings.bandMask = 0x06;
            settings.dwellUs = 2500;
            break;
        case Nrf24Jammer::SubGhz:
            settings.bandMask = 0x08;
            settings.dwellUs = 5000;
            break;
    }
    return settings;
}

bool RadioTuningModel::acceptsDataRate(uint16_t kbps) {
    return kbps == 250 || kbps == 1000 || kbps == 2000;
}

Nrf24TuningSettings RadioTuningModel::apply(Nrf24Jammer jammer,
                                            const Nrf24TuningSettings& desired,
                                            uint8_t& droppedControls) {
    const Nrf24TuningCapability capabilitySet = capabilityFor(jammer);
    Nrf24TuningSettings applied = defaults(jammer);
    droppedControls = 0;

    if (capabilitySet.spiClock) {
        applied.spiClockMhz = clampByte(desired.spiClockMhz, 1, 10);
    } else if (desired.spiClockMhz != applied.spiClockMhz) {
        ++droppedControls;
    }

    if (capabilitySet.power) {
        applied.powerDbm = nearestPowerStep(desired.powerDbm);
    } else if (desired.powerDbm != applied.powerDbm) {
        ++droppedControls;
    }

    if (capabilitySet.modulation) {
        applied.modulation = desired.modulation;
    } else if (desired.modulation != applied.modulation) {
        ++droppedControls;
    }

    if (capabilitySet.dataRate && acceptsDataRate(desired.dataRateKbps)) {
        applied.dataRateKbps = desired.dataRateKbps;
    } else if (desired.dataRateKbps != applied.dataRateKbps) {
        ++droppedControls;
    }

    if (capabilitySet.dwell) {
        applied.dwellUs = static_cast<uint16_t>(desired.dwellUs < 200 ? 200 : desired.dwellUs);
        if (applied.dwellUs > 20000) {
            applied.dwellUs = 20000;
        }
    } else if (desired.dwellUs != applied.dwellUs) {
        ++droppedControls;
    }

    if (capabilitySet.gfskPacketLength) {
        applied.gfskPacketLength = clampByte(desired.gfskPacketLength, 1, 32);
    } else if (desired.gfskPacketLength != applied.gfskPacketLength) {
        ++droppedControls;
    }

    if (capabilitySet.bandChannels) {
        applied.bandMask = static_cast<uint8_t>(desired.bandMask & 0x0F);
        if (applied.bandMask == 0) {
            // Never let a jammer be configured with no band at all.
            applied.bandMask = defaults(jammer).bandMask;
            ++droppedControls;
        }
    } else if (desired.bandMask != applied.bandMask) {
        ++droppedControls;
    }

    return applied;
}

uint8_t RadioTuningModel::channelForFrequencyMhz(uint16_t frequencyMhz) {
    if (frequencyMhz <= 2400) {
        return 0;
    }
    const uint16_t channel = static_cast<uint16_t>(frequencyMhz - 2400);
    return static_cast<uint8_t>(channel > 125 ? 125 : channel);
}

uint8_t RadioTuningModel::channelsForBand(Nrf24Band band, uint8_t* out, uint8_t capacity) {
    if (out == NULL || capacity == 0) {
        return 0;
    }
    uint8_t count = 0;
    switch (band) {
        case Nrf24Band::BleAdvertising: {
            // BLE advertising channels 37, 38 and 39.
            const uint16_t frequencies[3] = {2402, 2426, 2480};
            for (int index = 0; index < 3 && count < capacity; ++index) {
                out[count++] = channelForFrequencyMhz(frequencies[index]);
            }
            break;
        }
        case Nrf24Band::Wifi1_6_11: {
            const uint16_t frequencies[3] = {2412, 2437, 2462};
            for (int index = 0; index < 3 && count < capacity; ++index) {
                out[count++] = channelForFrequencyMhz(frequencies[index]);
            }
            break;
        }
        case Nrf24Band::Zigbee: {
            // 802.15.4 channels 11..26 step 5 MHz from 2405 MHz.
            for (uint16_t frequency = 2405; frequency <= 2480 && count < capacity; frequency += 5) {
                out[count++] = channelForFrequencyMhz(frequency);
            }
            break;
        }
        case Nrf24Band::IsmMisc: {
            // Fills the channels not covered by the named plans.
            for (uint8_t channel = 0; channel <= 125 && count < capacity; ++channel) {
                if (channel % 10 == 0) {
                    out[count++] = channel;
                }
            }
            break;
        }
    }
    return count;
}

const char* RadioTuningModel::jammerLabel(Nrf24Jammer jammer) {
    switch (jammer) {
        case Nrf24Jammer::Ble:
            return "BLE Jammer";
        case Nrf24Jammer::Wlan:
            return "WLAN Ember";
        case Nrf24Jammer::ProtoKill:
            return "Proto Kill";
        case Nrf24Jammer::SubGhz:
        default:
            return "SubGHz Scorch";
    }
}

const SubGhzBandSpec* RadioTuningModel::bands() {
    return kSubGhzBands;
}

uint8_t RadioTuningModel::bandCount() {
    return static_cast<uint8_t>(sizeof(kSubGhzBands) / sizeof(kSubGhzBands[0]));
}

bool RadioTuningModel::frequencySupported(uint32_t frequencyHz) {
    for (uint8_t index = 0; index < bandCount(); ++index) {
        const SubGhzBandSpec& band = kSubGhzBands[index];
        if (!band.offered) {
            continue;
        }
        if (frequencyHz >= band.lowHz && frequencyHz <= band.highHz) {
            return true;
        }
    }
    return false;
}

uint32_t RadioTuningModel::nearestSupportedBand(uint32_t frequencyHz) {
    uint32_t best = 433920000UL;
    uint32_t bestDistance = 0xFFFFFFFFUL;
    for (uint8_t index = 0; index < bandCount(); ++index) {
        const SubGhzBandSpec& band = kSubGhzBands[index];
        if (!band.offered || band.label[0] == 'W') {
            continue;  // WIDE is a sweep, not a tuning centre
        }
        const uint32_t centre = band.lowHz + (band.highHz - band.lowHz) / 2;
        const uint32_t distance = frequencyHz > centre ? frequencyHz - centre : centre - frequencyHz;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = centre;
        }
    }
    return best;
}

Cc1101TuningSettings RadioTuningModel::defaultsCc1101() {
    Cc1101TuningSettings settings;
    settings.frequencyOffsetHz = 0;
    return settings;
}

Cc1101TuningSettings RadioTuningModel::clampCc1101(const Cc1101TuningSettings& desired) {
    Cc1101TuningSettings applied;
    applied.modulation = desired.modulation > 4 ? 0 : desired.modulation;

    // CC1101 data rate is bounded by the crystal and modulation; 0.6 to 500
    // kBaud covers every mode this front end offers.
    applied.dataRateBaud = desired.dataRateBaud;
    if (applied.dataRateBaud < 600) {
        applied.dataRateBaud = 600;
    }
    if (applied.dataRateBaud > 500000) {
        applied.dataRateBaud = 500000;
    }

    // Offered receive filter bandwidths, rounded down to the nearest supported
    // step so the radio never claims a narrower filter than it has.
    const uint32_t bandwidths[6] = {58000, 68000, 100000, 203000, 270000, 500000};
    applied.rxBandwidthHz = bandwidths[0];
    for (int index = 0; index < 6; ++index) {
        if (desired.rxBandwidthHz >= bandwidths[index]) {
            applied.rxBandwidthHz = bandwidths[index];
        }
    }

    applied.agcSensitivity = clampByte(desired.agcSensitivity, 0, 7);

    applied.frequencyOffsetHz = desired.frequencyOffsetHz;
    if (applied.frequencyOffsetHz < -200000) {
        applied.frequencyOffsetHz = -200000;
    }
    if (applied.frequencyOffsetHz > 200000) {
        applied.frequencyOffsetHz = 200000;
    }
    return applied;
}

namespace {

class TuningCockpitFeature final : public IFeature {
public:
    TuningCockpitFeature(Nrf24Jammer jammer, const char* label) : jammer_(jammer), label_(label) {}

    FeatureResult begin(FeatureContext&) override {
        settings_ = RadioTuningModel::defaults(jammer_);
        return {FeatureResultCode::Completed, label_};
    }

    void tick() override {}

    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "tuning closed"}; }

private:
    Nrf24Jammer jammer_;
    const char* label_;
    Nrf24TuningSettings settings_;
};

}  // namespace

IFeature* createNrf24TuningFeature() {
    return new TuningCockpitFeature(Nrf24Jammer::Ble, "NRF tuning panel ready");
}

IFeature* createSubGhzTuningFeature() {
    return new TuningCockpitFeature(Nrf24Jammer::SubGhz, "SubGHz cockpit ready");
}
