#include "services/FeatureSwitchLedger.h"

uint32_t FeatureSwitchLedger::worstInVisit(const FeatureProfile& profile) {
    uint32_t worst = profile.beginMs;
    if (profile.tickMs > worst) {
        worst = profile.tickMs;
    }
    if (profile.endMs > worst) {
        worst = profile.endMs;
    }
    return worst;
}

// A BLE feature must be preceded by an actual teardown of the previous BLE
// feature. endMs == 0 for a BLE feature means it never leaves the stack.
bool FeatureSwitchLedger::bleHandoffMissing(const FeatureProfile& from,
                                            const FeatureProfile& to) {
    return from.usesBle && to.usesBle && from.endMs == 0U;
}

SwitchIssue FeatureSwitchLedger::replay(const FeatureProfile* const* order,
                                        size_t count) const {
    // An empty visit order touches nothing, so it cannot starve the renderer,
    // leak a resource or miss a teardown: it is clean by construction. Same
    // convention as worstWindowMs() below, which also tolerates a null order
    // when count is zero. A *non-empty* order with a null array is still a bug.
    if (count == 0U) {
        return clean();
    }
    if (order == nullptr) {
        return {SwitchIssueCode::InvalidProfile, "", "null visit order", 0U};
    }

    for (size_t i = 0; i < count; ++i) {
        const FeatureProfile* profile = order[i];
        if (profile == nullptr || profile->id == nullptr || profile->id[0] == '\0') {
            return {SwitchIssueCode::InvalidProfile, "", "visit has no id", 0U};
        }

        // 1. Time budget: any single window that starves the renderer.
        const uint32_t worst = worstInVisit(*profile);
        if (worst > renderBudgetMs_) {
            return {SwitchIssueCode::DisplayStarved, profile->id,
                    "one uninterrupted window exceeds the render budget", worst};
        }

        // 2. BLE hand-off: entering a BLE feature on top of a live one.
        if (i > 0 && bleHandoffMissing(*order[i - 1], *profile)) {
            return {SwitchIssueCode::TeardownMissing, profile->id,
                    "previous BLE feature never tore down", 0U};
        }

        // 3. Resource hygiene.
        if (profile->resourcesHeldAfterEnd != 0U) {
            return {SwitchIssueCode::ResourceLeak, profile->id,
                    "resource still leased after teardown",
                    profile->resourcesHeldAfterEnd};
        }
    }

    return clean();
}

uint32_t FeatureSwitchLedger::worstWindowMs(const FeatureProfile* const* order,
                                            size_t count) const {
    uint32_t worst = 0U;
    if (order == nullptr) {
        return worst;
    }
    for (size_t i = 0; i < count; ++i) {
        if (order[i] == nullptr) {
            continue;
        }
        const uint32_t candidate = worstInVisit(*order[i]);
        if (candidate > worst) {
            worst = candidate;
        }
    }
    return worst;
}
