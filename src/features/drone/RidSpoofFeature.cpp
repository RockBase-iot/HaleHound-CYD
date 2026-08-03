#include "features/drone/RemoteIdParser.h"

#include "core/FeatureRunner.h"

namespace {
class RidSpoofFeature final : public IFeature {
public:
    FeatureResult begin(FeatureContext&) override {
        return {FeatureResultCode::Blocked, "RID spoofing requires explicit offensive policy authorization"};
    }
    void tick() override {}
    void requestStop(StopReason) override {}
    FeatureResult end() override { return {FeatureResultCode::Completed, "RID spoof stopped"}; }
};
}

IFeature* createRidSpoofFeature() {
    return new RidSpoofFeature();
}
