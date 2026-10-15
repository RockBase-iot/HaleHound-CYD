#pragma once

#include <cstddef>
#include <cstdint>

// BLE module switching in any order, without white-screening the panel.
//
// Bench symptom this exists for: entering BLE features in arbitrary order
// (Jammer -> Spoofer -> AirTag hub -> Scanner -> SkeletonKey -> ...) leaves the
// panel blank. Two causes, both modelled here:
//
//   1. A feature never hands the UI back. Any single uninterrupted stretch
//      (BLE init, a discovery pass, a deinit drain) that exceeds the render
//      budget means the last frame stays on screen — the white screen.
//   2. A BLE feature is entered while the previous one is still inside the BLE
//      stack (no teardown), or leaves a shared resource leased afterwards.
//
// Everything is a declared cost profile, so a replay of any visit order is a
// pure function and every permutation can be checked.
struct FeatureProfile {
    const char* id;
    bool usesBle;
    uint32_t beginMs;              // BLE init / GATT setup, no UI
    uint32_t tickMs;              // longest single blocking stretch while running
    uint32_t endMs;               // deinit + callback drain, no UI
    uint32_t resourcesHeldAfterEnd;  // must be 0 — anything else is a leak
};

enum class SwitchIssueCode : uint8_t {
    Clean = 0,
    DisplayStarved,   // one uninterrupted window exceeded the render budget
    TeardownMissing,  // BLE feature entered before the previous one tore down
    ResourceLeak,     // a resource was still leased after teardown
    InvalidProfile,   // null/empty id
};

struct SwitchIssue {
    SwitchIssueCode code;
    const char* feature;  // the visit the issue was attributed to
    const char* detail;
    uint32_t observedMs;  // the offending window, 0 when not time-related

    bool clean() const { return code == SwitchIssueCode::Clean; }
};

class FeatureSwitchLedger {
public:
    static constexpr uint32_t kDefaultRenderBudgetMs = 250U;

    explicit FeatureSwitchLedger(uint32_t renderBudgetMs = kDefaultRenderBudgetMs)
        : renderBudgetMs_(renderBudgetMs == 0U ? kDefaultRenderBudgetMs
                                               : renderBudgetMs) {}

    // Replay a visit order. Returns the FIRST issue found; Clean means every
    // window stayed inside the budget, every BLE hand-off tore down, and no
    // resource survived its owning visit.
    SwitchIssue replay(const FeatureProfile* const* order, size_t count) const;

    uint32_t renderBudgetMs() const { return renderBudgetMs_; }
    // Longest uninterrupted window in a visit order — the white-screen metric.
    uint32_t worstWindowMs(const FeatureProfile* const* order, size_t count) const;

    static SwitchIssue clean() { return {SwitchIssueCode::Clean, "", "", 0U}; }

private:
    static uint32_t worstInVisit(const FeatureProfile& profile);
    static bool bleHandoffMissing(const FeatureProfile& from,
                                  const FeatureProfile& to);

    uint32_t renderBudgetMs_;
};
