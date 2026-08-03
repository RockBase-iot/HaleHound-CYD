#include "features/subghz/SubReadFeature.h"

#include "core/FeatureRunner.h"

namespace {
class TeslaChargeFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Unsupported,
                "Tesla Charge payload evidence and test vectors are unavailable"};
    }

    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override {
        return {FeatureResultCode::Completed, "Tesla Charge feature stopped"};
    }
};
}

TeslaChargeEvidence teslaChargeEvidence(TeslaChargeMode mode) {
    (void)mode;
    return {mode, false, "no reviewed Tesla Charge payload or test vector is bundled"};
}

IFeature* createTeslaChargeFeature() {
    return new TeslaChargeFeature();
}
