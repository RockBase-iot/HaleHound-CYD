#include "services/ResourceBroker.h"

bool resourceConflict(ResourceId left, ResourceId right) {
    return left == ResourceId::SharedSpi && right == ResourceId::SharedSpi;
}
