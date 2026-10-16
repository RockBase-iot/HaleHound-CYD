#pragma once

#include "core/FeatureTypes.h"

struct FeatureContext;
class IFeatureCanvas;

class IFeature {
public:
    virtual ~IFeature() = default;

    virtual FeatureResult begin(FeatureContext& context) = 0;
    virtual void tick() = 0;
    virtual void requestStop(StopReason reason) = 0;
    virtual FeatureResult end() = 0;

    // Paints this feature's own screen into `canvas`.
    //
    // Returns true when the feature owns the screen for this frame -- the
    // runner then reports ownsScreen() and the caller must not paint the legacy
    // chrome over it. The default paints nothing and returns false, which is the
    // honest answer for every feature whose screen is still drawn by the legacy
    // monolith, and for the catalog stubs, which have no screen at all.
    //
    // One override point rather than `drawsOwnScreen()` plus `draw()`: two
    // virtuals would be an invariant to keep in sync, and the failure mode of
    // drift is a feature that claims a screen it never paints.
    virtual bool draw(IFeatureCanvas& canvas) {
        (void)canvas;
        return false;
    }

    // True when the feature itself wants out -- its screen saw a cancel, or its
    // session reached a terminal state. The runner's owner polls this; a feature
    // that stops itself would otherwise never be stopped.
    //
    // Distinct from requestStop(), which is the owner telling the feature to
    // stop. This is the feature telling the owner. The legacy screens carry
    // exactly this signal (`X::isExitRequested()`) and it used to be consulted
    // by a hand-written loop per menu entry; routing it through the interface is
    // what lets one dispatch path serve every entry.
    virtual bool exitRequested() const { return false; }
};
