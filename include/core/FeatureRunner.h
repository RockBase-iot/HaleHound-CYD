#pragma once

#include "core/FeatureRegistry.h"
#include "core/IFeature.h"
#include "core/PolicyService.h"

class IFeatureCanvas;

struct FeatureContext {
    bool safeMode = false;

    // Where a running feature may paint, or nullptr when nothing is displaying
    // (host tests, a headless build). A feature must treat nullptr as "no
    // screen", not as "draw nothing and pretend it worked" -- the runner never
    // calls draw() without it.
    IFeatureCanvas* canvas = nullptr;
};

// The pre-flight gates. Every one of them is asked before a feature is
// constructed, so a feature never has to defend itself against a decision the
// runner could have made.
struct FeatureRunnerHooks {
    // Returns the policy *decision*, not a bool.
    //
    // `Authorization::detail` is the honest-report line, and `action` is what the
    // caller must do next. A bool collapses "offer the disclaimer" and "Blue Team
    // refuses outright" into one indistinguishable "blocked" -- and those two
    // need different screens, so the caller would have to re-ask policy itself
    // and become a second gate. Returning the decision keeps one authority.
    Authorization (*authorize)(const FeatureDescriptor& descriptor);

    // Route / peripheral / resource gates. A null hook means "this build has no
    // opinion", which is a different thing from "allowed": the repository has no
    // route model or resource broker wired into the runtime yet, and inventing a
    // refusal for a question nobody asked would be a lie in the other direction.
    bool (*routeSupported)(const FeatureDescriptor& descriptor);
    bool (*probePeripheral)(PeripheralId peripheral);
    bool (*preflightResources)(const FeatureDescriptor& descriptor);
};

class FeatureRunner {
public:
    FeatureRunner(const FeatureRegistry& registry,
                  FeatureContext& context,
                  FeatureRunnerHooks hooks);
    ~FeatureRunner();

    FeatureResult start(FeatureId id);
    void tick();
    void requestStop(StopReason reason);
    FeatureState state() const { return state_; }

    // True when the running feature painted its own frame on the last tick, so
    // the owner must not paint over it. False when nothing is running, when
    // there is no canvas, or when the feature declined the screen.
    bool ownsScreen() const { return ownsScreen_; }

    // True when the running feature has asked to end. The owner's loop uses this
    // as one of its exit conditions, alongside the user's own.
    bool exitRequested() const;

private:
    FeatureResult finish(FeatureResult result);
    void releaseFeature();

    const FeatureRegistry& registry_;
    FeatureContext& context_;
    FeatureRunnerHooks hooks_;
    IFeature* feature_ = nullptr;
    FeatureState state_ = FeatureState::Idle;
    bool endCalled_ = false;
    bool ownsScreen_ = false;
};
