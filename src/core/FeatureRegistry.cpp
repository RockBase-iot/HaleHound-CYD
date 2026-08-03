#include "core/FeatureRegistry.h"

const FeatureDescriptor* FeatureRegistry::find(FeatureId id) const {
    for (size_t index = 0; index < count_; ++index) {
        if (descriptors_[index].id == id) {
            return &descriptors_[index];
        }
    }
    return nullptr;
}

const FeatureDescriptor& FeatureRegistry::at(size_t index) const {
    return descriptors_[index];
}
