#include "features/ir/IrFeature.h"

#include "core/FeatureRunner.h"

namespace {
class IrFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Completed, "IR TX/RX feature ready; GPIO confirmation required"};
    }
    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "IR feature stopped"}; }
};
}

IFeature* createIrFeature() {
    return new IrFeature();
}
