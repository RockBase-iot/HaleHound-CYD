#include "features/FeatureMenuModel.h"

#include "core/FeatureRegistry.h"

uint8_t FeatureMenuModel::count(CategoryId category) {
    const FeatureRegistry& registry = featureCatalog();
    uint16_t found = 0;
    for (size_t index = 0; index < registry.size(); ++index) {
        if (registry.at(index).category == category) {
            ++found;
        }
    }
    return found > 0xFFU ? 0xFFU : static_cast<uint8_t>(found);
}

uint8_t FeatureMenuModel::build(CategoryId category, FeatureMenuItem* out,
                                uint8_t capacity) {
    if (out == nullptr || capacity == 0U) {
        return 0;
    }
    const FeatureRegistry& registry = featureCatalog();
    uint8_t written = 0;
    for (size_t index = 0; index < registry.size(); ++index) {
        if (written >= capacity) {
            break;
        }
        const FeatureDescriptor& descriptor = registry.at(index);
        if (descriptor.category != category) {
            continue;
        }
        out[written].id = descriptor.id;
        out[written].label = descriptor.displayName;
        out[written].risk = descriptor.risk;
        ++written;
    }
    return written;
}
