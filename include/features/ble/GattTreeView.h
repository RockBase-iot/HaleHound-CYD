#pragma once

// The GATT tree as the panel needs it.
//
// SkeletonTree is a data structure: nested vectors of services and
// characteristics. A 240-pixel-wide panel drawing a scrollable list needs the
// other shape -- one flat sequence of rows, with the collapsed state, the
// scroll position and the cursor folded in -- and needs it without allocating
// per frame.
//
// Doing that conversion in the renderer is how the two classic panel bugs
// happen: the cursor lands on a row that is not on screen (blank list), or it
// lands on a service row and the read/write buttons act on nothing. Both are
// decided here, where they can be asserted, rather than in a draw call.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "features/ble/SkeletonKeyModel.h"

struct GattTreeRow {
    // The sentinel for "this row is a service, not a characteristic".
    static constexpr uint16_t kNoCharacteristic = 0xFFFFU;

    uint8_t depth = 0;  // 0 = service, 1 = characteristic
    std::string uuid;
    std::string name;    // resolved by GattNameTable; empty when unknown
    std::string detail;  // the property letters, e.g. "R W N"
    std::string value;   // hex, empty until read
    uint16_t serviceIndex = 0;
    uint16_t characteristicIndex = kNoCharacteristic;
    bool expanded = false;  // services only: whether the children are listed
    bool hasChildren = false;
    bool truncated = false;

    bool isService() const { return characteristicIndex == kNoCharacteristic; }
    // The line the panel prints: the resolved name when there is one, and the
    // raw UUID when there is not. Never an invented name.
    std::string label() const { return name.empty() ? uuid : name; }
};

class GattTreeView {
public:
    static constexpr uint16_t kNoSelection = 0xFFFFU;

    // Loads a tree and starts with every service collapsed, which is what makes
    // a peer with sixteen services fit on the panel at all.
    void setTree(const SkeletonTree& tree);

    const SkeletonTree& tree() const { return tree_; }
    bool empty() const { return tree_.services.empty(); }
    bool truncated() const { return tree_.truncated; }

    // ── expansion ──────────────────────────────────────────────────────────
    void expand(uint16_t serviceIndex);
    void collapse(uint16_t serviceIndex);
    void toggle(uint16_t serviceIndex);
    void expandAll();
    void collapseAll();
    bool expanded(uint16_t serviceIndex) const;

    // ── rows ───────────────────────────────────────────────────────────────
    // The whole flat list honouring expansion, in screen order.
    std::vector<GattTreeRow> rows() const;
    uint16_t rowCount() const;

    // ── pagination ─────────────────────────────────────────────────────────
    // A page is how many rows the panel shows at once.
    void setPageSize(uint16_t rowsPerPage);
    uint16_t pageSize() const { return pageSize_; }
    uint16_t pageCount() const;
    uint16_t page() const { return page_; }
    void goToPage(uint16_t page);
    // Moves the cursor by `delta` rows and pulls the page along so the cursor is
    // never off screen -- the scroll position follows the cursor rather than
    // letting it leave the page.
    void scrollBy(int16_t delta);
    // The rows of the current page.
    std::vector<GattTreeRow> visibleRows() const;

    // ── selection ──────────────────────────────────────────────────────────
    // Any row can be selected, because a service row is what expand/collapse
    // acts on. Clamped into range; kNoSelection when there are no rows.
    void selectRow(uint16_t rowIndex);
    uint16_t selectedRow() const { return selected_; }
    // True only when the cursor is on a characteristic, and then the indices let
    // the caller address it. A service row or an empty list reports false, so a
    // read button cannot act on nothing.
    bool selectedCharacteristic(uint16_t& serviceIndex, uint16_t& characteristicIndex) const;

    // ── rendering helpers ──────────────────────────────────────────────────
    // The GATT property bits as the letters the dump uses, in declaration order:
    // "B R w W N I" (broadcast, read, write-without-response, write, notify,
    // indicate). Lower and upper 'w' are different capabilities on purpose.
    // Returns the characters written, excluding the terminator, and writes ""
    // for no properties. Never allocates.
    static std::size_t propertyLabel(uint8_t properties, char* out, std::size_t capacity);

private:
    // Keeps the cursor inside the row list after an expansion change shrinks it.
    void normaliseSelection();
    // Moves the page so it contains the cursor -- the invariant that keeps the
    // highlighted row from being off screen.
    void followPageToSelection();

    SkeletonTree tree_;
    std::vector<bool> expanded_;
    uint16_t pageSize_ = 8;
    uint16_t page_ = 0;
    uint16_t selected_ = kNoSelection;
};
