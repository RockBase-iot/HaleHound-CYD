#!/usr/bin/env python3
"""U2 wiring: AppKernel / IFeature / LegacyApp.inc.

Exact-string replacements with match-count assertions, so a silent no-op cannot
happen. Re-running an already-applied patch aborts harmlessly.

Usage: python build-reports/patch_u2_wiring.py
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEGACY = ROOT / "src" / "app" / "LegacyApp.inc"

applied: list[str] = []


def replace_once(text: str, name: str, old: str, new: str, expect: int = 1) -> str:
    found = text.count(old)
    if found != expect:
        sys.exit(
            f"{name}: expected {expect} match(es), found {found}. "
            "Nothing was written."
        )
    applied.append(name)
    return text.replace(old, new)


def cut_region(text: str, name: str, start_marker: str, end_marker: str, new: str) -> str:
    start = text.find(start_marker)
    if start < 0:
        sys.exit(f"{name}: start marker not found")
    end = text.find(end_marker, start)
    if end < 0:
        sys.exit(f"{name}: end marker not found after start")
    region = text[start:end]
    if len(region) < 3000:
        sys.exit(f"{name}: region is only {len(region)} chars -- refusing to cut")
    applied.append(f"{name} ({len(region)} chars removed)")
    return text[:start] + new + text[end:]


def main() -> int:
    text = LEGACY.read_text(encoding="utf-8", newline="")

    # ── P1: includes ────────────────────────────────────────────────────────
    text = replace_once(
        text,
        "P1 include AppKernel/FeatureMenuModel",
        '#include "core/PolicyService.h"\n#include "core/BootHealthService.h"\n'
        '#include "services/MenuLayout.h"\n#include "services/SettingsService.h"\n',
        '#include "core/AppKernel.h"\n#include "core/PolicyService.h"\n'
        '#include "core/BootHealthService.h"\n#include "features/FeatureCatalog.h"\n'
        '#include "features/FeatureMenuModel.h"\n'
        '#include "services/MenuLayout.h"\n#include "services/SettingsService.h"\n',
    )

    # ── P2: the policy store moves to the core ──────────────────────────────
    p2_start = "namespace {\n\nconstexpr int kPolicyEepromOffset = 128;"
    p2_end = "PolicyService& policy() {\n    static PolicyService service(policyStore());\n    return service;\n}"
    start = text.find(p2_start)
    if start < 0:
        sys.exit("P2: start marker not found")
    end = text.find(p2_end, start)
    if end < 0:
        sys.exit("P2: end marker not found")
    end += len(p2_end)
    removed = end - start
    if removed < 1000:
        sys.exit(f"P2: only {removed} chars to remove -- refusing to cut")
    applied.append(f"P2 policy store moved to core ({removed} chars removed)")
    text = text[:start] + (
        "// The policy instance and its store now live in the core\n"
        "// (src/core/PolicyService.cpp). They used to be defined here, which meant the\n"
        "// menu that renders the disclaimer and the FeatureRunner gate that decides\n"
        "// whether a feature may start could hold two stores and disagree -- the exact\n"
        "// failure PolicyService exists to prevent.\n"
        "//\n"
        "// The EEPROM layout did not change in the move: the Arduino backend in\n"
        "// PolicyService.cpp uses the same offset, size and CRC'd double-slot record\n"
        "// this region always had, so an already-provisioned device keeps its state.\n"
        "PolicyService& policy() { return globalPolicyService(); }\n"
        "SettingsService& policyStore() { return globalSettingsStore(); }\n"
    ) + text[end:]

    # ── P3: the Bluetooth submenu becomes catalog-derived ────────────────────
    p3_old = """// Bluetooth Submenu - 10 items
const int bluetooth_NUM_SUBMENU_ITEMS = 10;
const char *bluetooth_submenu_items[bluetooth_NUM_SUBMENU_ITEMS] = {
    "BLE Jammer",
    "BLE Spoofer",
    "BLE Beacon",
    "Sniffer",
    "BLE Scanner",
    "WhisperPair",
    "AirTag",
    "Lunatic Fringe",
    "BLE Ducky",
    "Back to Main Menu"
};

const unsigned char *bluetooth_submenu_icons[bluetooth_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_ble_jammer,
    bitmap_icon_spoofer,
    bitmap_icon_signal,
    bitmap_icon_analyzer,
    bitmap_icon_graph,
    bitmap_icon_eye,
    bitmap_icon_apple,
    bitmap_icon_scanner,
    bitmap_icon_key,            // BLE Ducky - key icon
    bitmap_icon_go_back
};
"""
    p3_new = """// Bluetooth Submenu — derived from the feature catalog at run time.
//
// This used to be a hand-kept array of ten names beside a hand-written ten-case
// switch. Both had drifted from the catalog: SkeletonKey, HID Reader, KARR,
// Airoha Race and BLE Predator are catalog entries under CategoryId::Bluetooth
// with no menu row at all, so five working screens were unreachable from the
// device, and the array's "Sniffer" matches no displayName in the catalog. A
// menu built from the catalog cannot drift from it.
//
// The label pointers point at the catalog's own strings (flash), so this costs
// three arrays of pointers and no string copies. The entries array is what the
// dispatch switches on, so a row's identity no longer depends on its index.
constexpr int kBluetoothMenuMax = 24;  // catalog entries + Back, with headroom
FeatureMenuItem bluetooth_menu_entries[kBluetoothMenuMax] = {};
const char *bluetooth_submenu_items[kBluetoothMenuMax] = {};
const unsigned char *bluetooth_submenu_icons[kBluetoothMenuMax] = {};
int bluetooth_NUM_SUBMENU_ITEMS = 0;

// Icons stay hand-mapped: the catalog carries no bitmap, and inventing one would
// drag a font down into features/. Anything unmapped gets the generic BLE glyph
// instead of, say, the previous row's icon by an off-by-one -- which is what a
// parallel index array would have produced the first time the two lists differed
// in length.
const unsigned char *bluetoothMenuIcon(FeatureId id) {
    switch (id) {
        case FeatureId::BluetoothJammer:       return bitmap_icon_ble_jammer;
        case FeatureId::BluetoothSpoofer:      return bitmap_icon_spoofer;
        case FeatureId::BluetoothBeacon:       return bitmap_icon_signal;
        case FeatureId::BluetoothSniffer:      return bitmap_icon_analyzer;
        case FeatureId::BluetoothScanner:      return bitmap_icon_graph;
        case FeatureId::BluetoothWhisperPair:  return bitmap_icon_eye;
        case FeatureId::BluetoothAirTag:       return bitmap_icon_apple;
        case FeatureId::BluetoothLunaticFringe: return bitmap_icon_scanner;
        case FeatureId::BluetoothDucky:        return bitmap_icon_key;
        default:                               return bitmap_icon_skull_bluetooth;
    }
}
"""
    text = replace_once(text, "P3 catalog-derived Bluetooth rows", p3_old, p3_new)

    # ── P4: the builder, right after the active-submenu pointers ────────────
    p4_anchor = """// Active submenu pointers
const char **active_submenu_items = nullptr;
int active_submenu_size = 0;
const unsigned char **active_submenu_icons = nullptr;
"""
    p4_new = p4_anchor + """
// Fills the Bluetooth rows from the catalog and points the active-submenu
// globals at them. Rebuilt on every entry to the submenu rather than once at
// boot, because the AirTag hub repoints those globals at its own tables and the
// pointers have to survive that.
void setupBluetoothSubmenu() {
    const uint8_t count = FeatureMenuModel::build(
        CategoryId::Bluetooth, bluetooth_menu_entries,
        static_cast<uint8_t>(kBluetoothMenuMax - 1));

    uint8_t row = 0;
    for (; row < count; ++row) {
        bluetooth_submenu_items[row] = bluetooth_menu_entries[row].label;
        bluetooth_submenu_icons[row] = bluetoothMenuIcon(bluetooth_menu_entries[row].id);
    }
    // Back is appended here rather than emitted by the model: it is a menu
    // affordance, not a feature, and a model that emitted it would have to invent
    // a FeatureId for it. Its row index is therefore count, which is also what
    // derives the constants downstream code needs.
    bluetooth_submenu_items[row] = "Back to Main Menu";
    bluetooth_submenu_icons[row] = bitmap_icon_go_back;
    bluetooth_NUM_SUBMENU_ITEMS = static_cast<int>(row) + 1;

    active_submenu_items = bluetooth_submenu_items;
    active_submenu_size = bluetooth_NUM_SUBMENU_ITEMS;
    active_submenu_icons = bluetooth_submenu_icons;
}
"""
    text = replace_once(text, "P4 setupBluetoothSubmenu()", p4_anchor, p4_new)

    # ── P5: forward declarations for the catalog dispatch ──────────────────
    text = replace_once(
        text,
        "P5 forward declarations",
        "void handleBluetoothSubmenuTouch();\nvoid handleAirTagHubTouch();\n",
        "void handleBluetoothSubmenuTouch();\nvoid handleAirTagHubTouch();\n"
        "void runCatalogFeature(FeatureId id);\n",
    )

    # ── P6: updateActiveSubmenu case 1 ─────────────────────────────────────
    text = replace_once(
        text,
        "P6 updateActiveSubmenu case 1",
        "        case 1: // Bluetooth\n"
        "            active_submenu_items = bluetooth_submenu_items;\n"
        "            active_submenu_size = bluetooth_NUM_SUBMENU_ITEMS;\n"
        "            active_submenu_icons = bluetooth_submenu_icons;\n"
        "            break;\n",
        "        case 1: // Bluetooth — rows come from the catalog (setupBluetoothSubmenu)\n"
        "            setupBluetoothSubmenu();\n"
        "            break;\n",
    )

    # ── P7: the AirTag hub's restore ───────────────────────────────────────
    text = replace_once(
        text,
        "P7 AirTag hub restore",
        "    // Restore Bluetooth submenu pointers for caller's returnToSubmenu()\n"
        "    active_submenu_items = bluetooth_submenu_items;\n"
        "    active_submenu_size = bluetooth_NUM_SUBMENU_ITEMS;\n"
        "    active_submenu_icons = bluetooth_submenu_icons;\n",
        "    // Restore Bluetooth submenu pointers for caller's returnToSubmenu()\n"
        "    setupBluetoothSubmenu();\n",
    )

    # ── P8: the ten-case dispatch becomes one call ─────────────────────────
    p8_end = (
        "            returnToSubmenu();\n"
        "            break;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "\n"
        "// ═══════════════════════════════════════════════════════════════════════════\n"
        "// AIRTAG HUB SUBMENU HANDLER"
    )
    p8_new = """            // Back is the last row by construction -- setupBluetoothSubmenu appends
            // it -- so the index is derived instead of the literal 9 it used to be,
            // which was only right while the array happened to hold ten rows.
            if (current_submenu_index == bluetooth_NUM_SUBMENU_ITEMS - 1) {
                returnToMainMenu();
                return;
            }

            waitForTouchRelease();
            runCatalogFeature(bluetooth_menu_entries[current_submenu_index].id);
"""
    text = cut_region(
        text,
        "P8 catalog dispatch",
        "            if (current_submenu_index == 9) { // Back",
        p8_end,
        p8_new,
    )

    # ── P9: the dispatch machinery itself ──────────────────────────────────
    p9_old = """bool requestOffensiveAccess() {
    const Authorization auth = policy().authorizeOffensive();
    switch (auth.action) {
        case PolicyAction::Proceed:
            return true;
        case PolicyAction::PromptValhalla:
            // Undecided: the disclaimer is the unlock path.
            return showDisclaimerScreen();
        case PolicyAction::BlockedBlueTeam:
        default:
            showBlueTeamBlockedScreen();
            // Deliberately no disclaimer offer here. Blue Team has exactly one
            // exit and it is a separate, acknowledged flow in Device Policy.
            return false;
    }
}
"""
    p9_new = """// The one place the three PolicyActions turn into pixels. Everything that gates
// on policy funnels through here, so a fourth action would be a compile warning
// rather than a fifth screen that disagrees with the other four.
bool resolvePolicyAction(const Authorization& auth) {
    switch (auth.action) {
        case PolicyAction::Proceed:
            return true;
        case PolicyAction::PromptValhalla:
            // Undecided: the disclaimer is the unlock path.
            return showDisclaimerScreen();
        case PolicyAction::BlockedBlueTeam:
        default:
            showBlueTeamBlockedScreen();
            // Deliberately no disclaimer offer here. Blue Team has exactly one
            // exit and it is a separate, acknowledged flow in Device Policy.
            return false;
    }
}

bool requestOffensiveAccess() {
    // For call sites with no catalog entry to name: the policy's own offensive
    // probe. Call sites that do have a descriptor use policyPrecheck(), which
    // asks about that descriptor rather than about a synthetic one.
    return resolvePolicyAction(policy().authorizeOffensive());
}

// The menu's pre-check. Deliberately the same authority the runner will consult
// -- but asked for the *action*, because "offer the disclaimer" and "Blue Team
// refuses" are two different screens and the runner reduces both to Blocked.
//
// This is not the gate. The runner re-asks policy for the decision, so a call
// path that forgets this pre-check still cannot start an offensive feature; what
// forgetting it costs is the right screen, not the guarantee.
bool policyPrecheck(const FeatureDescriptor& descriptor) {
    return resolvePolicyAction(policy().authorize(descriptor));
}

// The honest-report screen. A feature that refused to start says why in words:
// "required peripheral missing" tells the user something, and a silent return to
// the menu tells them the button is broken.
void showFeatureRefusal(const char* title, const char* detail) {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, VALHALLA_BLUE);
    tft.setTextColor(VALHALLA_BLUE);
    tft.setTextSize(2);
    tft.setCursor(12, 60);
    tft.print(title == nullptr ? "Refused" : title);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(12, 110);
    tft.print(detail == nullptr ? "no detail reported" : detail);
    delay(1600);
}

// Runs one catalog entry to completion. This is the single dispatch path the
// whole redesign exists for: it replaced ten hand-written `case` blocks, each
// calling X::setup()/loop()/cleanup() with its own copy of the exit condition,
// and none of which ever touched the runner, the catalog or the policy gate.
void runCatalogFeature(FeatureId id) {
    const FeatureDescriptor* descriptor = featureCatalog().find(id);
    if (descriptor == nullptr) {
        showFeatureRefusal("Not registered", "the catalog has no such feature");
        return;
    }

    // The AirTag row opens a hub of three sessions rather than one session. A
    // menu is not a feature, so it keeps its own screen -- but the row exists
    // because the catalog has it, which is the property that was missing before.
    if (id == FeatureId::BluetoothAirTag) {
        handleAirTagHubTouch();
        return;
    }

    if (!policyPrecheck(*descriptor)) {
        return;
    }

    FeatureRunner& runner = globalAppKernel().runner();
    const FeatureResult started = runner.start(id);
    if (started.code != FeatureResultCode::Completed) {
        // Blocked already rendered its own screen inside policyPrecheck, and
        // re-rendering a second one would hide the first.
        if (started.code != FeatureResultCode::Blocked) {
            showFeatureRefusal("Cannot start", started.detail);
        }
        return;
    }

    feature_active = true;
    feature_exit_requested = false;
    while (!feature_exit_requested) {
        runner.tick();
        // The feature's own exit signal. It used to be consulted by hand in
        // every case block; routing it through IFeature means one loop serves
        // every entry, including ones added to the catalog later.
        if (runner.exitRequested()) break;
        touchButtonsUpdate();
        if (isBackButtonTapped()) break;
        if (IS_BOOT_PRESSED()) break;
    }
    feature_active = false;
    feature_exit_requested = false;
    runner.requestStop(StopReason::UserRequest);
}
"""
    text = replace_once(text, "P9 dispatch machinery", p9_old, p9_new)

    LEGACY.write_text(text, encoding="utf-8", newline="")
    print(f"patched {LEGACY.relative_to(ROOT)}:")
    for name in applied:
        print(f"  ok  {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
