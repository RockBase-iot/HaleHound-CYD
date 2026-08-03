#pragma once

#include <cstddef>

#include "core/FeatureTypes.h"
#include "features/FeatureCatalog.h"

class FeatureRegistry {
public:
    FeatureRegistry(const FeatureDescriptor* descriptors, size_t count)
        : descriptors_(descriptors), count_(count) {}

    const FeatureDescriptor* find(FeatureId id) const;
    const FeatureDescriptor& at(size_t index) const;
    size_t size() const { return count_; }

private:
    const FeatureDescriptor* descriptors_;
    size_t count_;
};
