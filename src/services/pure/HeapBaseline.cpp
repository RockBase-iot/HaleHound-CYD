#include "services/HeapBaseline.h"

namespace {

bool parseU32(const char* text, uint32_t& out) {
    if (text == nullptr || *text < '0' || *text > '9') {
        return false;
    }
    uint64_t value = 0;
    while (*text >= '0' && *text <= '9') {
        value = value * 10U + static_cast<uint64_t>(*text - '0');
        if (value > 0xFFFFFFFFULL) {
            return false;
        }
        ++text;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

// Looks for "key=" and returns the pointer just past it, or nullptr.
const char* findField(const char* line, const char* key) {
    if (line == nullptr || key == nullptr) {
        return nullptr;
    }
    for (const char* cursor = line; *cursor != '\0'; ++cursor) {
        const char* a = cursor;
        const char* b = key;
        while (*a != '\0' && *b != '\0' && *a == *b) {
            ++a;
            ++b;
        }
        if (*b == '\0' && *a == '=') {
            return a + 1;
        }
    }
    return nullptr;
}

size_t copyBoardName(const char* value, char* out, size_t capacity) {
    size_t written = 0;
    if (value == nullptr) {
        return 0;
    }
    while (value[written] != '\0' && value[written] != ' ' && written + 1U < capacity) {
        out[written] = value[written];
        ++written;
    }
    out[written] = '\0';
    return written;
}

}  // namespace

int32_t HeapBaselineCheck::regressionX100(uint32_t reference, uint32_t current) {
    if (reference == 0U) {
        return 0;
    }
    if (current >= reference) {
        return -static_cast<int32_t>(
            (static_cast<uint64_t>(current - reference) * 10000ULL) / reference);
    }
    return static_cast<int32_t>(
        (static_cast<uint64_t>(reference - current) * 10000ULL) / reference);
}

HeapAssessment HeapBaselineCheck::assess(const HeapBaseline& reference,
                                         const HeapSample& current) const {
    HeapAssessment result{};
    result.freeDelta = static_cast<int32_t>(current.freeHeap) -
                       static_cast<int32_t>(reference.freeHeap);

    if (reference.board[0] == '\0') {
        result.verdict = HeapVerdict::Invalid;
        result.detail = "baseline has no board tag";
        return result;
    }
    if (reference.freeHeap == 0U) {
        result.verdict = HeapVerdict::Invalid;
        result.detail = "baseline free heap is zero";
        return result;
    }

    result.percentDeltaX100 =
        regressionX100(reference.freeHeap, current.freeHeap);
    result.percentShrinkX100 =
        regressionX100(reference.largestBlock, current.largestBlock);

    // Absolute floor first: below it the TLS/HTTP paths fail regardless of how
    // good the relative number looks.
    if (current.freeHeap < floorBytes_) {
        result.verdict = HeapVerdict::Critical;
        result.detail = "free heap below the absolute floor";
        return result;
    }

    if (result.percentDeltaX100 >= static_cast<int32_t>(criticalX100_)) {
        result.verdict = HeapVerdict::Critical;
        result.detail = "free heap regressed past the critical threshold";
        return result;
    }

    if (result.percentDeltaX100 >= static_cast<int32_t>(warnX100_)) {
        result.verdict = HeapVerdict::Warn;
        result.detail = "free heap regressed past the warn threshold";
        return result;
    }

    // Fracture check: the largest allocatable block must stay a usable
    // fraction of the free total, otherwise allocation fails while the totals
    // still look healthy.
    if (current.largestBlock * kFragRatio < current.freeHeap) {
        result.verdict = HeapVerdict::Warn;
        result.detail = "heap too fragmented for a large allocation";
        return result;
    }

    if (result.percentShrinkX100 >= static_cast<int32_t>(criticalX100_)) {
        result.verdict = HeapVerdict::Warn;
        result.detail = "largest free block shrank sharply";
        return result;
    }

    result.verdict = HeapVerdict::WithinBudget;
    result.detail = "within budget";
    return result;
}

size_t formatHeapBaseline(const HeapBaseline& baseline, char* out, size_t capacity) {
    if (out == nullptr || capacity == 0U) {
        return 0;
    }
    // Hand-rolled u32 -> decimal so the formatter has no stdio dependency and
    // produces byte-identical output on host and target.
    char buffer[160];
    size_t used = 0;
    const char* fields[3] = {" free=", " minFree=", " largest="};
    const uint32_t values[3] = {baseline.freeHeap, baseline.minFreeHeap,
                                baseline.largestBlock};

    auto append = [&](const char* text) {
        while (*text != '\0' && used + 1U < sizeof(buffer)) {
            buffer[used++] = *text++;
        }
    };
    auto appendU32 = [&](uint32_t value) {
        char digits[11];
        size_t count = 0;
        if (value == 0U) {
            digits[count++] = '0';
        }
        while (value > 0U && count < sizeof(digits)) {
            digits[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        }
        while (count > 0U && used + 1U < sizeof(buffer)) {
            buffer[used++] = digits[--count];
        }
    };

    append("board=");
    const char* board = baseline.board[0] != '\0' ? baseline.board : "unknown";
    append(board);
    for (size_t i = 0; i < 3; ++i) {
        append(fields[i]);
        appendU32(values[i]);
    }

    const size_t copy = used < capacity - 1U ? used : capacity - 1U;
    for (size_t i = 0; i < copy; ++i) {
        out[i] = buffer[i];
    }
    out[copy] = '\0';
    return copy;
}

bool parseHeapBaseline(const char* line, HeapBaseline& out) {
    if (line == nullptr) {
        return false;
    }
    out = HeapBaseline{};
    copyBoardName(findField(line, "board"), out.board, sizeof(out.board));
    if (out.board[0] == '\0') {
        return false;
    }
    return parseU32(findField(line, "free"), out.freeHeap) &&
           parseU32(findField(line, "minFree"), out.minFreeHeap) &&
           parseU32(findField(line, "largest"), out.largestBlock);
}
