#include "features/drone/RemoteIdParser.h"

#include "core/FeatureRunner.h"

namespace {
class DroneJellyFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Blocked, "Drone Jelly requires explicit offensive policy authorization"};
    }
    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "Drone Jelly stopped"}; }
};
}

IFeature* createDroneJellyFeature() {
    return new DroneJellyFeature();
}
