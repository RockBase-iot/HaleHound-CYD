#include "services/FirmwareImageValidator.h"

#include <cstring>

namespace {
constexpr size_t kImageHeaderSize = 24U;
constexpr size_t kSegmentHeaderSize = 8U;
constexpr uint8_t kImageMagic = 0xE9U;
constexpr uint8_t kMaxSegments = 16U;

uint16_t read16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0]) |
           static_cast<uint16_t>(bytes[1] << 8U);
}

uint32_t read32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8U) |
           (static_cast<uint32_t>(bytes[2]) << 16U) |
           (static_cast<uint32_t>(bytes[3]) << 24U);
}

uint16_t chipId(FirmwareChip chip) {
    return chip == FirmwareChip::Esp32C5 ? 23U : 0U;
}

ImageValidationResult failure(FirmwareChip expected, ImageValidationError error,
                              size_t length, uint8_t segments = 0U,
                              uint16_t minRevision = 0U,
                              uint16_t maxRevision = 0U) {
    return {false, expected, static_cast<uint32_t>(length), segments,
            minRevision, maxRevision, error};
}
}

ImageValidationResult FirmwareImageValidator::validate(const IImageReader& image,
                                                        FirmwareChip expectedChip,
                                                        uint32_t partitionBytes,
                                                        const char*) {
    const size_t fileSize = image.size();
    if (fileSize < 1U) return failure(expectedChip, ImageValidationError::FileTooShort, fileSize);
    uint8_t magic = 0U;
    if (!image.read(0U, &magic, 1U)) {
        return failure(expectedChip, ImageValidationError::FileTooShort, fileSize);
    }
    if (magic != kImageMagic) {
        return failure(expectedChip, ImageValidationError::BadMagic, fileSize);
    }
    if (fileSize < kImageHeaderSize) {
        return failure(expectedChip, ImageValidationError::HeaderTruncated, fileSize);
    }

    uint8_t header[kImageHeaderSize]{};
    if (!image.read(0U, header, sizeof(header))) {
        return failure(expectedChip, ImageValidationError::HeaderTruncated, fileSize);
    }
    const uint8_t segmentCount = header[1];
    if (segmentCount == 0U || segmentCount > kMaxSegments) {
        return failure(expectedChip, ImageValidationError::InvalidSegmentCount,
                       fileSize, segmentCount);
    }
    const uint16_t actualChip = read16(header + 12U);
    if (actualChip != chipId(expectedChip)) {
        return failure(expectedChip, ImageValidationError::WrongChip,
                       fileSize, segmentCount, read16(header + 14U), read16(header + 16U));
    }
    if (fileSize > partitionBytes) {
        return failure(expectedChip, ImageValidationError::PartitionTooSmall,
                       fileSize, segmentCount, read16(header + 14U), read16(header + 16U));
    }

    size_t offset = kImageHeaderSize;
    for (uint8_t segment = 0U; segment < segmentCount; ++segment) {
        if (offset > fileSize || kSegmentHeaderSize > fileSize - offset) {
            return failure(expectedChip, ImageValidationError::SegmentTableTruncated,
                           fileSize, segmentCount, read16(header + 14U), read16(header + 16U));
        }
        uint8_t segmentHeader[kSegmentHeaderSize]{};
        if (!image.read(offset, segmentHeader, sizeof(segmentHeader))) {
            return failure(expectedChip, ImageValidationError::SegmentTableTruncated,
                           fileSize, segmentCount, read16(header + 14U), read16(header + 16U));
        }
        const uint32_t dataLength = read32(segmentHeader + 4U);
        offset += kSegmentHeaderSize;
        if (dataLength > fileSize - offset) {
            return failure(expectedChip, ImageValidationError::SegmentOutOfBounds,
                           fileSize, segmentCount, read16(header + 14U), read16(header + 16U));
        }
        offset += dataLength;
    }
    if (offset > fileSize) {
        return failure(expectedChip, ImageValidationError::DeclaredLengthMismatch,
                       fileSize, segmentCount, read16(header + 14U), read16(header + 16U));
    }

    return {true, expectedChip, static_cast<uint32_t>(fileSize), segmentCount,
            read16(header + 14U), read16(header + 16U), ImageValidationError::Ok};
}
