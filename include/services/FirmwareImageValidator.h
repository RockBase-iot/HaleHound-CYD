#pragma once

#include <cstddef>
#include <cstdint>

enum class FirmwareChip : uint8_t {
    Esp32,
    Esp32C5,
};

enum class ImageValidationError : uint8_t {
    Ok,
    FileTooShort,
    BadMagic,
    InvalidSegmentCount,
    HeaderTruncated,
    WrongChip,
    SegmentTableTruncated,
    SegmentOutOfBounds,
    DeclaredLengthMismatch,
    PartitionTooSmall,
};

struct ImageValidationResult {
    bool valid;
    FirmwareChip targetChip;
    uint32_t imageLength;
    uint8_t segmentCount;
    uint16_t minRevision;
    uint16_t maxRevision;
    ImageValidationError error;
};

class IImageReader {
public:
    virtual ~IImageReader() = default;
    virtual size_t size() const = 0;
    virtual bool read(size_t offset, void* destination, size_t length) const = 0;
};

class FirmwareImageValidator {
public:
    static ImageValidationResult validate(const IImageReader& image,
                                          FirmwareChip expectedChip,
                                          uint32_t partitionBytes,
                                          const char* filename);
};
