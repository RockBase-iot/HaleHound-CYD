#pragma once

#include <cstddef>
#include <cstdint>

enum class StoragePathId : uint8_t {
    Subghz,
    Eapol,
    Wardriving,
    WpLoot,
    Loot,
    Credentials,
    CredsInput,
    IotReconInput,
    Firmware,
};

namespace storage_paths {
const char* canonical(StoragePathId id);
bool isReadOnly(StoragePathId id);
}

enum class PathError : uint8_t {
    None,
    Empty,
    TooLong,
    Traversal,
    Backslash,
    UnknownRoot,
    ReadOnly,
};

struct PathCheck {
    bool valid;
    PathError error;
    const char* canonical;
};

class PathPolicy {
public:
    static constexpr size_t kMaxPathLength = 96U;
    static PathCheck validate(const char* path, bool writing);
};

bool migrateCredentialPath(const char* source,
                           char* destination,
                           size_t destinationCapacity);
bool isLegacyPskRecord(const char* line);
