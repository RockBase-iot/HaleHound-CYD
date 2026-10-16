#pragma once

#include <cstddef>
#include <cstdint>

// Heap baseline accounting for the bench regression run.
//
// The bench records one sample per board per build and compares it against a
// stored baseline. "It still boots" is not a heap result; a percentage
// regression with a named verdict is. All arithmetic is integer — a float here
// would make the verdict depend on the FPU and stop being reproducible.
struct HeapBaseline {
    char board[24] = {0};
    uint32_t freeHeap = 0;
    uint32_t minFreeHeap = 0;
    uint32_t largestBlock = 0;
};

struct HeapSample {
    uint32_t freeHeap = 0;
    uint32_t minFreeHeap = 0;
    uint32_t largestBlock = 0;
};

enum class HeapVerdict : uint8_t {
    WithinBudget = 0,
    Warn,      // measurable regression, still shippable
    Critical,  // blocks release
    Invalid,   // unusable input (zero baseline, no board)
};

struct HeapAssessment {
    HeapVerdict verdict = HeapVerdict::Invalid;
    int32_t freeDelta = 0;         // current - baseline; negative = regression
    int32_t percentDeltaX100 = 0;  // +1245 == a 12.45 % regression
    int32_t percentShrinkX100 = 0; // largest-block shrink vs baseline
    const char* detail = "no assessment";
};

class HeapBaselineCheck {
public:
    static constexpr uint32_t kDefaultWarnX100 = 800U;      //  8.00 %
    static constexpr uint32_t kDefaultCriticalX100 = 1500U;  // 15.00 %
    static constexpr uint32_t kDefaultFloorBytes = 24576U;   // 24 KiB
    // largestBlock * kFragRatio < freeHeap  => the heap is too fractured to
    // satisfy a TLS buffer even though the free total looks fine.
    static constexpr uint32_t kFragRatio = 4U;

    explicit HeapBaselineCheck(uint32_t warnPercentX100 = kDefaultWarnX100,
                               uint32_t criticalPercentX100 = kDefaultCriticalX100,
                               uint32_t absoluteFloorBytes = kDefaultFloorBytes)
        : warnX100_(warnPercentX100),
          criticalX100_(criticalPercentX100),
          floorBytes_(absoluteFloorBytes) {}

    HeapAssessment assess(const HeapBaseline& reference,
                          const HeapSample& current) const;

    // (reference - current) * 10000 / reference, as hundredths of a percent.
    static int32_t regressionX100(uint32_t reference, uint32_t current);

    uint32_t warnPercentX100() const { return warnX100_; }
    uint32_t criticalPercentX100() const { return criticalX100_; }
    uint32_t floorBytes() const { return floorBytes_; }

private:
    uint32_t warnX100_;
    uint32_t criticalX100_;
    uint32_t floorBytes_;
};

// One-line storage format, so the bench script can read and write a baseline
// without a JSON dependency on either side:
//   board=nm-cyd-c5 free=143216 minFree=118944 largest=65536
size_t formatHeapBaseline(const HeapBaseline& baseline, char* out, size_t capacity);
bool parseHeapBaseline(const char* line, HeapBaseline& out);
