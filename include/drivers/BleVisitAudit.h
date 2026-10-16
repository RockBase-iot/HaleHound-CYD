#pragma once

// Auditing one BLE visit, and feeding the result to the two models that decide
// whether a switch is safe.
//
// FeatureSwitchLedger takes a *declared* cost profile. HeapBaselineCheck takes
// a *measured* heap sample. Neither can see a live controller, which is why the
// ledger's `resourcesHeldAfterEnd` has been a hand-written constant since it
// was written -- and a constant is exactly what cannot catch a session that
// forgets to hand the stack back.
//
// This is the seam that closes that gap: it reads the controller's own
// accounting (`connected()`, `scanning()`, `subscriptionCount()`) instead of
// tracking a parallel copy, so a session cannot report itself clean while the
// transport disagrees.
//
// Why the counters live here rather than on IBleCentral: begin() is idempotent
// in the stack, so "started twice, ended once" is invisible from the transport
// alone. Wrapping adds that pair-counting without forcing a new method on every
// implementation -- including the one that has not been written yet.

#include <cstddef>
#include <cstdint>

#include "drivers/IBleCentral.h"
#include "services/FeatureSwitchLedger.h"
#include "services/HeapBaseline.h"

// What a controller is holding right now. Read off the transport, never
// remembered alongside it.
struct BleResourceState {
    bool running = false;   // begin() with no matching end()
    bool connected = false;
    bool scanning = false;
    uint8_t subscriptions = 0;
    uint16_t opens = 0;  // begin() calls the session made
    uint16_t closes = 0; // end() calls the session made

    // Everything still leased. A scan is one lease and each subscription is one
    // more; the controller is *one* lease however it is being read -- `running`
    // and `connected` are two views of the same lease, not two leases.
    uint32_t held() const;
    // The only safe value after a teardown.
    bool released() const { return held() == 0U; }
    // Balanced bring-ups and teardowns. Distinct from released(): a session can
    // call end() and still leave a subscription standing, because end() is not
    // obliged to unsubscribe.
    bool balanced() const { return opens == closes; }
    // "running, connected, 2 subscription(s)" -- for the honest-report line.
    // False when the buffer is short, leaving `out` empty rather than cut.
    bool describe(char* out, std::size_t capacity) const;
};

// Where heap numbers come from on the device. One method, so the host suite can
// script a leaking visit instead of needing a board.
class IBleHeapProbe {
public:
    virtual ~IBleHeapProbe() = default;
    virtual HeapSample sampleHeap() const = 0;
};

// The heap cost of one visit: idle before it, idle after it.
struct BleHeapBudget {
    // current - reference, in bytes. Negative means the visit did not give the
    // heap back.
    int32_t leakedBytes = 0;
    // The same as hundredths of a percent of the pre-visit free heap, computed
    // with the same integer arithmetic the baseline check uses, so the two
    // cannot disagree about how bad a regression is.
    int32_t percentDeltaX100 = 0;
    uint32_t beforeFreeHeap = 0;
    uint32_t afterFreeHeap = 0;
};

BleHeapBudget measureBleVisit(const HeapSample& before, const HeapSample& after);

// A declared profile with its resource count replaced by what the transport
// actually reported. A declared 0 that the hardware contradicts is precisely
// the failure the ledger exists to catch, so the measurement wins and the
// declaration is kept only for the timing fields.
FeatureProfile profileFromMeasurement(const FeatureProfile& declared,
                                      const BleResourceState& measured);

// The verdict the bench prints for a post-visit sample. Delegates to the
// baseline check so there is one definition of "8 % is a warning".
HeapAssessment assessBleVisit(const HeapBaseline& reference, const HeapSample& after);

// One visit, audited end to end. `beginVisit()` is called by the feature's own
// begin() and `endVisit()` by its end(); everything else is read back.
class BleVisitAudit {
public:
    BleVisitAudit(IBleCentral& central, IBleHeapProbe& heap)
        : central_(central), heap_(heap) {}

    void beginVisit();
    void endVisit();

    BleResourceState resources() const;
    BleHeapBudget heapCost() const;

    // True only when the visit handed everything back *and* left the heap no
    // smaller than it found it. Two independent facts, both required.
    bool clean() const;

    // The one-line bench record: resources held, bytes not returned.
    // False when the buffer is short, leaving `out` empty.
    bool describe(char* out, std::size_t capacity) const;

    uint16_t visitCount() const { return visits_; }

private:
    IBleCentral& central_;
    IBleHeapProbe& heap_;
    uint16_t opens_ = 0;
    uint16_t closes_ = 0;
    uint16_t visits_ = 0;
    HeapSample before_{};
    HeapSample after_{};
};
