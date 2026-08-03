#include "services/StoragePaths.h"

#include <cstring>

bool migrateCredentialPath(const char* source,
                           char* destination,
                           size_t destinationCapacity) {
    if (source == nullptr || destination == nullptr || destinationCapacity == 0U) {
        return false;
    }
    if (std::strcmp(source, "/creds.txt") != 0) {
        return false;
    }
    const char* target = "/loot/credentials/legacy_creds.txt";
    const size_t length = std::strlen(target);
    if (length + 1U > destinationCapacity) {
        return false;
    }
    std::memcpy(destination, target, length + 1U);
    return true;
}

bool isLegacyPskRecord(const char* line) {
    return line != nullptr && std::strncmp(line, "[PSK]", 5U) == 0;
}
