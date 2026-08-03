#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IFeature;

enum class BlePredatorPhase : uint8_t {
    Idle,
    Scan,
    Recon,
    Honeypot,
};

struct BleWriteEvent {
    std::string address;
    std::string characteristic;
    std::string value;
};

class BlePredatorModel {
public:
    BlePredatorPhase phase() const { return phase_; }

    bool advance(BlePredatorPhase next) {
        if (next == BlePredatorPhase::Scan && phase_ == BlePredatorPhase::Idle) {
            phase_ = next;
            return true;
        }
        if (next == BlePredatorPhase::Recon && phase_ == BlePredatorPhase::Scan) {
            phase_ = next;
            return true;
        }
        if (next == BlePredatorPhase::Honeypot && phase_ == BlePredatorPhase::Recon) {
            phase_ = next;
            return true;
        }
        return false;
    }

    bool recordWrite(const std::string& address,
                     const std::string& characteristic,
                     const std::string& value) {
        if (phase_ != BlePredatorPhase::Honeypot || address.empty() || characteristic.empty()) {
            return false;
        }
        writeEvents_.push_back({address, characteristic, value});
        return true;
    }

    const std::vector<BleWriteEvent>& writeEvents() const { return writeEvents_; }
    const char* lootPath() const { return "/wp_loot"; }
    void releasePhase() {
        phase_ = BlePredatorPhase::Idle;
        writeEvents_.clear();
    }

private:
    BlePredatorPhase phase_ = BlePredatorPhase::Idle;
    std::vector<BleWriteEvent> writeEvents_;
};

IFeature* createBlePredatorFeature();
IFeature* createAirohaRaceFeature();
