#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IFeature;

struct SkimmerEvidence {
    bool nameMatch = false;
    bool serviceMatch = false;
    bool behaviorMatch = false;
    int rssi = -127;
};

struct SkimmerScore {
    uint8_t value = 0;
    bool likelySkimmer = false;
    std::vector<std::string> reasons;
};

class SkimmerScorer {
public:
    static SkimmerScore evaluate(const SkimmerEvidence& evidence);
};

class SkimmerHuntSession {
public:
    bool start(bool userConfirmed) {
        active_ = userConfirmed;
        return active_;
    }
    void stop() { active_ = false; }
    bool active() const { return active_; }

private:
    bool active_ = false;
};

IFeature* createSkimmerHuntFeature();
