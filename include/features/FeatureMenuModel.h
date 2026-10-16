#pragma once

#include <cstdint>

#include "core/FeatureTypes.h"
#include "features/FeatureCatalog.h"

// A menu row, taken from the catalog rather than from a hand-kept array.
struct FeatureMenuItem {
    FeatureId id = FeatureId::WifiScanner;
    // Points at the descriptor's own storage -- the catalog is a compile-time
    // table of string literals, so this is flash, not a copy. Never nullptr for
    // a row this model produced.
    const char* label = nullptr;
    RiskClass risk = RiskClass::Passive;
};

// Builds a category's menu from the feature catalog.
//
// Why this exists: the legacy submenus were hand-kept arrays of names next to a
// hand-written `switch` on the row index, and the catalog was a third list. By
// the time this was written the three had drifted -- Bluetooth had fourteen
// catalog entries and nine menu rows, so SkeletonKey, HID Reader, KARR, Airoha
// Race and BLE Predator were unreachable from the screen while "Sniffer" in the
// array matched no `displayName`. A menu derived from the catalog cannot drift
// from it, and the host suite can assert the menu shows every entry rather than
// a human having to notice.
//
// Deliberately no drawing, no Arduino, no pixels: the row list is a pure
// function of the catalog, so it is host-testable, and the screen decides how a
// row looks.
class FeatureMenuModel {
public:
    // Writes the catalog's entries for `category` into `out`, in catalog order,
    // and returns how many were written.
    //
    // The "Back" row is not included: it is a menu affordance rather than a
    // feature, and a model that emitted it would have to invent a FeatureId for
    // it. The caller appends it at index `count`.
    //
    // Never writes past `capacity`. A category with more entries than the
    // caller's array is reported by a short count -- the caller compares against
    // `count(category)` -- rather than by a truncated menu silently presented as
    // the whole category.
    static uint8_t build(CategoryId category, FeatureMenuItem* out, uint8_t capacity);

    // How many rows `category` has, so a caller can size an array without
    // guessing and a test can assert nothing was dropped.
    static uint8_t count(CategoryId category);
};
