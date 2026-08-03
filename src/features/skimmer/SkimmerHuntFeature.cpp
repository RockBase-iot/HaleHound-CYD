#include "features/skimmer/SkimmerScore.h"

#include "core/FeatureRunner.h"

SkimmerScore SkimmerScorer::evaluate(const SkimmerEvidence& evidence) {
    SkimmerScore result;
    if (evidence.nameMatch) {
        result.value = static_cast<uint8_t>(result.value + 45U);
        result.reasons.emplace_back("name pattern matched");
    }
    if (evidence.serviceMatch) {
        result.value = static_cast<uint8_t>(result.value + 35U);
        result.reasons.emplace_back("service UUID matched");
    }
    if (evidence.behaviorMatch) {
        result.value = static_cast<uint8_t>(result.value + 20U);
        result.reasons.emplace_back("connection behavior matched");
    }
    if (evidence.rssi >= -55) {
        result.value = static_cast<uint8_t>(result.value > 95U ? 100U : result.value + 5U);
        result.reasons.emplace_back("near-field RSSI");
    }
    result.likelySkimmer = result.value >= 70U;
    return result;
}

namespace {
class SkimmerFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        session_.start(false);
        return {FeatureResultCode::Completed, "Skimmer scoring ready; active confirmation required"};
    }
    void tick() override {}
    void requestStop(StopReason) override { session_.stop(); }
    FeatureResult end() override {
        session_.stop();
        return {FeatureResultCode::Completed, "Skimmer hunt stopped"};
    }

private:
    SkimmerHuntSession session_;
};
}

IFeature* createSkimmerHuntFeature() {
    return new SkimmerFeature();
}
