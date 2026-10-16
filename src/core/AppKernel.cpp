#include "core/AppKernel.h"

#include "app/LegacyApp.h"
#include "core/PolicyService.h"
#include "features/FeatureCatalog.h"

namespace {
LegacyCallbacks defaultLegacyCallbacks() {
    return {legacySetup, legacyLoop, nullptr, nullptr};
}

// ─── The gates the kernel installs ─────────────────────────────────────────
//
// Free functions, because FeatureRunnerHooks carries plain function pointers:
// there is one policy authority and one catalog, so a context pointer would
// only add a way to reach a different one.
Authorization policyAuthorize(const FeatureDescriptor& descriptor) {
    return globalPolicyService().authorize(descriptor);
}

FeatureRunnerHooks runtimeHooks() {
    // Only the policy gate is wired. `routeSupported`, `probePeripheral` and
    // `preflightResources` stay null on purpose: the repository has no route
    // model, no peripheral probe and no resource broker bound at runtime yet,
    // and a hook that answered "yes" to a question nobody asked would be a
    // louder lie than a null hook the runner documents as having no opinion.
    //
    // The gate that matters is installed: before this, the kernel passed `{}`
    // and FeatureRunner treated a null authorize as "allowed", so every one of
    // the catalog's offensive entries started without ever consulting
    // PolicyService. The service was correct and unreachable.
    return {policyAuthorize, nullptr, nullptr, nullptr};
}
}

AppKernel::AppKernel()
    : legacyAdapter_(defaultLegacyCallbacks()),
      runner_(featureCatalog(), context_, runtimeHooks()) {}

AppKernel::AppKernel(LegacyCallbacks callbacks)
    : legacyAdapter_(callbacks),
      runner_(featureCatalog(), context_, runtimeHooks()) {}

AppKernel& globalAppKernel() {
    static AppKernel kernel;
    return kernel;
}
