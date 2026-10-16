#include "features/ble/GattTreeView.h"

namespace {

// The property letters, in declaration order. 'w' and 'W' are deliberately
// different: write-without-response and write are different capabilities, and a
// panel that showed them as one letter would tell the user a write will be
// acknowledged when it will not.
struct PropertyLetter {
    uint8_t bit;
    char letter;
};

constexpr PropertyLetter kPropertyLetters[] = {
    {0x01, 'B'},  // broadcast
    {0x02, 'R'},  // read
    {0x04, 'w'},  // write without response
    {0x08, 'W'},  // write
    {0x10, 'N'},  // notify
    {0x20, 'I'},  // indicate
};

}  // namespace

std::size_t GattTreeView::propertyLabel(uint8_t properties, char* out, std::size_t capacity) {
    if (out == nullptr || capacity == 0U) {
        return 0U;
    }
    std::size_t written = 0;
    for (const PropertyLetter& entry : kPropertyLetters) {
        if ((properties & entry.bit) == 0U) {
            continue;
        }
        if (written + 2U > capacity) {
            // No room for the letter plus its terminator: stop rather than write
            // a half-formatted label.
            break;
        }
        if (written > 0U) {
            out[written++] = ' ';
        }
        out[written++] = entry.letter;
    }
    out[written] = '\0';
    return written;
}

// ── layout ────────────────────────────────────────────────────────────────

void GattTreeView::setTree(const SkeletonTree& tree) {
    tree_ = tree;
    expanded_.assign(tree_.services.size(), false);
    page_ = 0;
    // A panel with nothing highlighted looks broken, so the cursor starts on the
    // first row when there is one.
    selected_ = tree_.services.empty() ? kNoSelection : 0U;
}

void GattTreeView::expand(uint16_t serviceIndex) {
    if (serviceIndex >= expanded_.size()) {
        return;
    }
    expanded_[serviceIndex] = true;
    normaliseSelection();
    followPageToSelection();
}

void GattTreeView::collapse(uint16_t serviceIndex) {
    if (serviceIndex >= expanded_.size()) {
        return;
    }
    expanded_[serviceIndex] = false;
    normaliseSelection();
    followPageToSelection();
}

void GattTreeView::toggle(uint16_t serviceIndex) {
    if (serviceIndex >= expanded_.size()) {
        return;
    }
    expanded_[serviceIndex] = !expanded_[serviceIndex];
    normaliseSelection();
    followPageToSelection();
}

void GattTreeView::expandAll() {
    expanded_.assign(tree_.services.size(), true);
    normaliseSelection();
    followPageToSelection();
}

void GattTreeView::collapseAll() {
    expanded_.assign(tree_.services.size(), false);
    normaliseSelection();
    followPageToSelection();
}

bool GattTreeView::expanded(uint16_t serviceIndex) const {
    return serviceIndex < expanded_.size() && expanded_[serviceIndex];
}

// ── rows ──────────────────────────────────────────────────────────────────

namespace {

void fillServiceRow(const SkeletonTree& tree, uint16_t serviceIndex, bool expanded,
                    GattTreeRow& out) {
    const SkeletonService& service = tree.services[serviceIndex];
    out = GattTreeRow{};
    out.depth = 0;
    out.uuid = service.uuid;
    out.name = service.name;
    out.value.clear();
    out.detail.clear();
    out.serviceIndex = serviceIndex;
    out.characteristicIndex = GattTreeRow::kNoCharacteristic;
    out.expanded = expanded;
    out.hasChildren = !service.characteristics.empty();
    out.truncated = service.truncated;
}

void fillCharacteristicRow(const SkeletonTree& tree, uint16_t serviceIndex,
                           uint16_t characteristicIndex, GattTreeRow& out) {
    const SkeletonCharacteristic& characteristic =
        tree.services[serviceIndex].characteristics[characteristicIndex];
    out = GattTreeRow{};
    out.depth = 1;
    out.uuid = characteristic.uuid;
    out.name = characteristic.name;
    out.value = characteristic.lastValue;
    char label[16] = {};
    GattTreeView::propertyLabel(characteristic.properties, label, sizeof(label));
    out.detail = label;
    out.serviceIndex = serviceIndex;
    out.characteristicIndex = characteristicIndex;
}

std::size_t appendServiceRows(const SkeletonTree& tree, uint16_t serviceIndex, bool isExpanded,
                              GattTreeRow* out, std::size_t written, std::size_t stopAfter) {
    if (written >= stopAfter) {
        return written;
    }
    fillServiceRow(tree, serviceIndex, isExpanded, out[written]);
    ++written;
    if (!isExpanded) {
        return written;
    }
    const std::size_t children = tree.services[serviceIndex].characteristics.size();
    for (std::size_t child = 0; child < children; ++child) {
        if (written >= stopAfter) {
            break;
        }
        fillCharacteristicRow(tree, serviceIndex, static_cast<uint16_t>(child), out[written]);
        ++written;
    }
    return written;
}

}  // namespace

std::vector<GattTreeRow> GattTreeView::rows() const {
    const uint16_t count = rowCount();
    std::vector<GattTreeRow> out;
    out.resize(count);
    std::size_t written = 0;
    for (uint16_t serviceIndex = 0; serviceIndex < tree_.services.size(); ++serviceIndex) {
        written = appendServiceRows(tree_, serviceIndex, expanded(serviceIndex), out.data(),
                                    written, static_cast<std::size_t>(count));
    }
    return out;
}

uint16_t GattTreeView::rowCount() const {
    std::size_t total = 0;
    for (uint16_t serviceIndex = 0; serviceIndex < tree_.services.size(); ++serviceIndex) {
        ++total;  // the service row itself
        if (expanded(serviceIndex)) {
            total += tree_.services[serviceIndex].characteristics.size();
        }
    }
    return static_cast<uint16_t>(total);
}

namespace {

// Walks to the `index`-th row without materialising the whole list.
bool rowAt(const SkeletonTree& tree, const std::vector<bool>& expanded, uint16_t index,
           GattTreeRow& out) {
    uint16_t cursor = 0;
    for (uint16_t serviceIndex = 0; serviceIndex < tree.services.size(); ++serviceIndex) {
        const bool isExpanded = serviceIndex < expanded.size() && expanded[serviceIndex];
        if (cursor == index) {
            fillServiceRow(tree, serviceIndex, isExpanded, out);
            return true;
        }
        ++cursor;
        if (!isExpanded) {
            continue;
        }
        const uint16_t children =
            static_cast<uint16_t>(tree.services[serviceIndex].characteristics.size());
        for (uint16_t child = 0; child < children; ++child) {
            if (cursor == index) {
                fillCharacteristicRow(tree, serviceIndex, child, out);
                return true;
            }
            ++cursor;
        }
    }
    return false;
}

}  // namespace

// ── pagination ────────────────────────────────────────────────────────────

void GattTreeView::setPageSize(uint16_t rowsPerPage) {
    // A zero-row page would divide by zero in followPageToSelection, and is not
    // a page in any case.
    pageSize_ = rowsPerPage == 0U ? 1U : rowsPerPage;
    normaliseSelection();
    followPageToSelection();
}

uint16_t GattTreeView::pageCount() const {
    const uint16_t count = rowCount();
    if (count == 0U) {
        return 0U;
    }
    return static_cast<uint16_t>((count + pageSize_ - 1U) / pageSize_);
}

void GattTreeView::goToPage(uint16_t page) {
    const uint16_t pages = pageCount();
    if (pages == 0U) {
        page_ = 0;
        selected_ = kNoSelection;
        return;
    }
    if (page >= pages) {
        page = static_cast<uint16_t>(pages - 1U);
    }
    page_ = page;
    // Jumping pages moves the cursor to that page's first row, so the invariant
    // "the cursor is always on screen" holds for a page jump too.
    selected_ = static_cast<uint16_t>(page_ * pageSize_);
    normaliseSelection();
}

void GattTreeView::scrollBy(int16_t delta) {
    const uint16_t count = rowCount();
    if (count == 0U) {
        selected_ = kNoSelection;
        page_ = 0;
        return;
    }
    int32_t target = selected_ == kNoSelection ? 0 : static_cast<int32_t>(selected_) + delta;
    if (target < 0) {
        target = 0;
    }
    if (target >= count) {
        target = count - 1;
    }
    selectRow(static_cast<uint16_t>(target));
}

std::vector<GattTreeRow> GattTreeView::visibleRows() const {
    const uint16_t count = rowCount();
    std::vector<GattTreeRow> out;
    if (count == 0U) {
        return out;
    }
    const uint32_t start = static_cast<uint32_t>(page_) * pageSize_;
    const uint32_t end = start + pageSize_;
    out.reserve(pageSize_);
    for (uint32_t index = start; index < end && index < count; ++index) {
        GattTreeRow row;
        if (rowAt(tree_, expanded_, static_cast<uint16_t>(index), row)) {
            out.push_back(row);
        }
    }
    return out;
}

// ── selection ─────────────────────────────────────────────────────────────

void GattTreeView::selectRow(uint16_t rowIndex) {
    const uint16_t count = rowCount();
    if (count == 0U) {
        selected_ = kNoSelection;
        page_ = 0;
        return;
    }
    if (rowIndex >= count) {
        rowIndex = static_cast<uint16_t>(count - 1U);
    }
    selected_ = rowIndex;
    followPageToSelection();
}

bool GattTreeView::selectedCharacteristic(uint16_t& serviceIndex,
                                          uint16_t& characteristicIndex) const {
    const uint16_t count = rowCount();
    if (selected_ == kNoSelection || selected_ >= count) {
        return false;
    }
    GattTreeRow row;
    if (!rowAt(tree_, expanded_, selected_, row) || row.isService()) {
        return false;
    }
    serviceIndex = row.serviceIndex;
    characteristicIndex = row.characteristicIndex;
    return true;
}

void GattTreeView::normaliseSelection() {
    if (rowCount() == 0U) {
        selected_ = kNoSelection;
        return;
    }
    if (selected_ == kNoSelection) {
        selected_ = 0U;
        return;
    }
    if (selected_ >= rowCount()) {
        selected_ = static_cast<uint16_t>(rowCount() - 1U);
    }
}

void GattTreeView::followPageToSelection() {
    const uint16_t count = rowCount();
    if (count == 0U || selected_ == kNoSelection) {
        page_ = 0;
        return;
    }
    page_ = static_cast<uint16_t>(selected_ / pageSize_);
    const uint16_t pages = pageCount();
    if (pages != 0U && page_ >= pages) {
        page_ = static_cast<uint16_t>(pages - 1U);
    }
}
