#include "drivers/BleVisitAudit.h"

namespace {

// Fixed-buffer text assembly, hand-rolled for the same reason formatHeapBaseline
// is: no stdio dependency, and byte-identical output on host and target.
class Text {
public:
    Text(char* out, std::size_t capacity) : out_(out), capacity_(capacity) {
        usable_ = out_ != nullptr && capacity_ >= 2U;
        // Empty on every failure path, including the one where there is room for
        // a terminator and nothing else. A caller must never be able to read a
        // cut line as a complete one.
        if (out_ != nullptr && capacity_ > 0U) {
            out_[0] = '\0';
        }
    }

    bool usable() const { return usable_; }

    void append(const char* text) {
        if (!usable_ || text == nullptr) {
            return;
        }
        while (*text != '\0') {
            put(*text++);
        }
    }

    void appendU32(uint32_t value) {
        char digits[10];
        std::size_t count = 0;
        if (value == 0U) {
            digits[count++] = '0';
        }
        while (value > 0U && count < sizeof(digits)) {
            digits[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        }
        while (count > 0U) {
            put(digits[--count]);
        }
    }

    void appendI32(int32_t value) {
        if (value < 0) {
            put('-');
            // Negating INT32_MIN would overflow; the magnitudes here are heap
            // deltas, so shifting through int64 is both safe and honest.
            appendU32(static_cast<uint32_t>(-static_cast<int64_t>(value)));
            return;
        }
        appendU32(static_cast<uint32_t>(value));
    }

    // True when everything fitted. On overflow the buffer is cleared, so a
    // caller cannot mistake a truncated line for a complete one.
    bool commit() {
        if (!usable_ || overflowed_) {
            if (out_ != nullptr && capacity_ > 0U) {
                out_[0] = '\0';
            }
            return false;
        }
        out_[used_] = '\0';
        return true;
    }

private:
    void put(char character) {
        if (!usable_ || overflowed_) {
            return;
        }
        if (used_ + 1U >= capacity_) {
            overflowed_ = true;
            return;
        }
        out_[used_++] = character;
    }

    char* out_;
    std::size_t capacity_;
    std::size_t used_ = 0;
    bool overflowed_ = false;
    bool usable_ = false;
};

}  // namespace

uint32_t BleResourceState::held() const {
    uint32_t count = subscriptions;
    // One controller, however many ways it is still held: `running` (the visit
    // never ended) and `connected` (the link is up) are two readings of the same
    // lease, so counting both would report four leases for three objects and
    // make the bench line wrong. They stay separate fields because they are
    // different facts -- only the count collapses them.
    if (running || connected) {
        ++count;
    }
    if (scanning) {
        ++count;
    }
    return count;
}

bool BleResourceState::describe(char* out, std::size_t capacity) const {
    Text text(out, capacity);
    if (!text.usable()) {
        return false;
    }

    if (released()) {
        text.append("released");
    } else {
        bool first = true;
        auto part = [&](const char* label) {
            if (!first) {
                text.append(", ");
            }
            first = false;
            text.append(label);
        };
        if (running) {
            part("running");
        }
        if (connected) {
            part("connected");
        }
        if (scanning) {
            part("scanning");
        }
        if (subscriptions > 0U) {
            if (!first) {
                text.append(", ");
            }
            first = false;
            text.appendU32(subscriptions);
            text.append(" subscription(s)");
        }
        text.append(" still held");
    }

    if (!balanced()) {
        // The half of the story the transport cannot tell: begin() is
        // idempotent, so an unpaired bring-up leaves no other trace.
        text.append(" [begin ");
        text.appendU32(opens);
        text.append(" / end ");
        text.appendU32(closes);
        text.append("]");
    }
    return text.commit();
}

BleHeapBudget measureBleVisit(const HeapSample& before, const HeapSample& after) {
    BleHeapBudget budget;
    budget.beforeFreeHeap = before.freeHeap;
    budget.afterFreeHeap = after.freeHeap;
    budget.leakedBytes = static_cast<int32_t>(after.freeHeap) -
                         static_cast<int32_t>(before.freeHeap);
    // The same integer arithmetic the baseline check uses, so a visit's cost and
    // the bench verdict cannot disagree about how bad the same number is.
    budget.percentDeltaX100 =
        HeapBaselineCheck::regressionX100(before.freeHeap, after.freeHeap);
    return budget;
}

FeatureProfile profileFromMeasurement(const FeatureProfile& declared,
                                     const BleResourceState& measured) {
    // The measurement wins outright. The declaration supplies the timing fields,
    // which no transport can report, and nothing else.
    FeatureProfile profile = declared;
    profile.resourcesHeldAfterEnd = measured.held();
    return profile;
}

HeapAssessment assessBleVisit(const HeapBaseline& reference, const HeapSample& after) {
    HeapBaselineCheck check;
    return check.assess(reference, after);
}

void BleVisitAudit::beginVisit() {
    ++opens_;
    ++visits_;
    before_ = heap_.sampleHeap();
    // Until endVisit() lands, the "after" reading is the "before" one -- a visit
    // that has not finished has not cost anything yet.
    after_ = before_;
}

void BleVisitAudit::endVisit() {
    ++closes_;
    after_ = heap_.sampleHeap();
}

BleResourceState BleVisitAudit::resources() const {
    BleResourceState state;
    state.running = opens_ > closes_;
    state.connected = central_.connected();
    state.scanning = central_.scanning();
    state.subscriptions = central_.subscriptionCount();
    state.opens = opens_;
    state.closes = closes_;
    return state;
}

BleHeapBudget BleVisitAudit::heapCost() const { return measureBleVisit(before_, after_); }

bool BleVisitAudit::clean() const {
    // Two independent facts. A visit can hand every lease back and still have
    // fragmented the heap, and it can leave a lease standing with the heap
    // untouched; either one is a failed visit.
    return resources().released() && heapCost().leakedBytes >= 0;
}

bool BleVisitAudit::describe(char* out, std::size_t capacity) const {
    Text text(out, capacity);
    if (!text.usable()) {
        return false;
    }
    text.append("held=");
    text.appendU32(resources().held());
    text.append(" heapDelta=");
    text.appendI32(heapCost().leakedBytes);
    text.append("B (");
    text.appendI32(heapCost().percentDeltaX100);
    text.append(" x100)");
    if (!resources().balanced()) {
        text.append(" UNBALANCED");
    }
    return text.commit();
}
