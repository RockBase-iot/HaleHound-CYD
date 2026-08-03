#include "features/drone/RemoteIdParser.h"

#include "core/FeatureRunner.h"

namespace {
class DroneDetectFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Completed, "Remote ID passive detection ready"};
    }
    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "Remote ID detection stopped"}; }
};
}

IFeature* createDroneDetectFeature() {
    return new DroneDetectFeature();
}
