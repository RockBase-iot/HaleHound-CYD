#!/usr/bin/env python3
"""One-shot patch for src/app/LegacyApp.inc.

Applies the PolicyService single-point-authorization收口 as a set of exact
block replacements with hard count assertions, so a half-applied edit can never
land. Line endings are preserved (the file is mixed CRLF/LF).

Run with --dry-run to only report match counts.
"""
from __future__ import annotations

import re
import sys

TARGET = r"E:\github\HaleHound-CYD\src\app\LegacyApp.inc"


def pat_for(block: str) -> re.Pattern[str]:
    parts = [re.escape(p) for p in block.split("\n")]
    return re.compile(r"\r?\n".join(parts))


def eol_of(sample: str) -> str:
    return "\r\n" if "\r\n" in sample else "\n"


def newline_block(block: str, eol: str) -> str:
    return block.replace("\n", eol)


class Patcher:
    def __init__(self, text: str) -> None:
        self.text = text
        self.log: list[str] = []

    def sub_exact(self, label: str, old: str, new: str, expect: int = 1) -> None:
        pattern = pat_for(old)
        matches = list(pattern.finditer(self.text))
        if len(matches) != expect:
            raise SystemExit(
                f"[FAIL] {label}: expected {expect} match(es), found {len(matches)}"
            )
        # single pass, right-to-left so offsets stay valid
        for m in reversed(matches):
            eol = eol_of(m.group(0))
            self.text = self.text[: m.start()] + newline_block(new, eol) + self.text[m.end():]
        self.log.append(f"  ok  {label}  x{expect}")

    def replace_line(self, label: str, old_line: str, new_block: str, expect: int = 1) -> None:
        self.sub_exact(label, old_line, new_block, expect)

    def sub_first_after(self, label: str, fn_anchor: str, old: str, new: str) -> None:
        """Replace the first match at/after `fn_anchor` — used when the same
        snippet appears in several handlers."""
        start = self.text.find(fn_anchor)
        if start < 0:
            raise SystemExit(f"[FAIL] {label}: anchor {fn_anchor!r} not found")
        m = pat_for(old).search(self.text, start)
        if m is None:
            raise SystemExit(f"[FAIL] {label}: no match after {fn_anchor!r}")
        eol = eol_of(m.group(0))
        self.text = self.text[: m.start()] + newline_block(new, eol) + self.text[m.end():]
        self.log.append(f"  ok  {label}  x1 (scoped)")

    def insert_after_line(self, label: str, anchor: str, block: str, expect: int = 1) -> None:
        pattern = re.compile(re.escape(anchor))
        matches = list(pattern.finditer(self.text))
        if len(matches) != expect:
            raise SystemExit(
                f"[FAIL] {label}: anchor matched {len(matches)} time(s), expected {expect}"
            )
        m = matches[-1]
        end = m.end()
        if self.text[end:end + 2] == "\r\n":
            after = end + 2
            eol = "\r\n"
        elif self.text[end:end + 1] == "\n":
            after = end + 1
            eol = "\n"
        else:
            after = end
            eol = eol_of(self.text[max(0, m.start() - 40):m.start()])
        self.text = self.text[:after] + newline_block(block, eol) + self.text[after:]
        self.log.append(f"  ok  {label}  x{expect}")


# ── 1. includes ────────────────────────────────────────────────────────────
INCLUDES = '''#include "services/SettingsService.h"
#include <EEPROM.h>
#include <cstring>
'''

# ── 2. legacy globals become mirrors ───────────────────────────────────────
GLOBALS_OLD = '''// VALHALLA Protocol — Liability disclaimer + blue team mode
bool disclaimer_accepted = false;   // Legal disclaimer accepted (persisted in EEPROM)
bool blue_team_mode = false;        // VALHALLA blue team mode active (persisted in EEPROM)
'''
GLOBALS_NEW = '''// VALHALLA Protocol — liability disclaimer + blue team mode.
// MIRRORS of PolicyService state, never an authority. mirrorPolicyToGlobals()
// is their only writer after boot; they exist so the rest of the legacy surface
// (home-screen banner, legacy EEPROM struct) keeps reading something coherent.
bool disclaimer_accepted = false;
bool blue_team_mode = false;
'''

# ── 3. policy store + app-level policy API ─────────────────────────────────
POLICY_BLOCK = '''// ═══════════════════════════════════════════════════════════════════════════
// DEVICE POLICY STORE — PolicyService is the single authorization authority
// ═══════════════════════════════════════════════════════════════════════════
//
// Before this change every offensive menu entry carried its own copy of the
// gate and mutated the two globals directly, and showDisclaimerScreen() cleared
// blue_team_mode on ACCEPT — so any blocked tool could be unlocked by simply
// re-accepting the disclaimer. Policy state now lives in a dedicated
// double-slot + CRC EEPROM region, PolicyService owns every transition, and
// every gate funnels through requestOffensiveAccess().

void saveSettings();
void loadSettings();

namespace {

constexpr int kPolicyEepromOffset = 128;  // reserved: 2 x sizeof(SettingsRecord)
constexpr int kPolicyEepromSize = 512;

int policySlotOffset(uint8_t slot) {
    return kPolicyEepromOffset +
           static_cast<int>(slot) * static_cast<int>(sizeof(SettingsRecord));
}

// EEPROM-backed ISettingsBackend for the policy region only. Deliberately
// separate from the legacy Settings struct so PolicyService can own its own
// record (magic + schema + CRC + sequence) without colliding with the old
// single-blob layout.
class EepromPolicyBackend final : public ISettingsBackend {
public:
    bool read(uint8_t slot, void* data, size_t size) override {
        if (slot > 1U || data == nullptr || size != sizeof(SettingsRecord)) {
            return false;
        }
        SettingsRecord record{};
        EEPROM.begin(kPolicyEepromSize);
        EEPROM.get(policySlotOffset(slot), record);
        EEPROM.end();
        std::memcpy(data, &record, sizeof(record));
        return true;
    }

    bool write(uint8_t slot, const void* data, size_t size) override {
        if (slot > 1U || data == nullptr || size != sizeof(SettingsRecord)) {
            return false;
        }
        SettingsRecord record{};
        std::memcpy(&record, data, sizeof(record));
        EEPROM.begin(kPolicyEepromSize);
        EEPROM.put(policySlotOffset(slot), record);
        const bool ok = EEPROM.commit();
        EEPROM.end();
        return ok;
    }

    bool commit() override {
        EEPROM.begin(kPolicyEepromSize);
        const bool ok = EEPROM.commit();
        EEPROM.end();
        return ok;
    }
};

}  // namespace

SettingsService& policyStore() {
    static EepromPolicyBackend backend;
    static SettingsService store(backend);
    return store;
}

PolicyService& policy() {
    static PolicyService service(policyStore());
    return service;
}

// The only writer of the legacy mirrors, and the only path that keeps the
// legacy EEPROM bytes in step with the policy store.
void mirrorPolicyToGlobals() {
    disclaimer_accepted = policy().disclaimerAccepted();
    blue_team_mode = policy().inBlueTeam();
    saveSettings();
}

// Adopt the legacy EEPROM bytes exactly once, then never look at them again.
void policyBootstrap() {
    SettingsService& store = policyStore();
    store.load();
    if (!store.hasStoredRecord()) {
        store.migrateLegacy(disclaimer_accepted, blue_team_mode);
    }
    policy().syncFromStore();
    disclaimer_accepted = policy().disclaimerAccepted();
    blue_team_mode = policy().inBlueTeam();
}

bool policyAcceptValhalla() {
    if (!policy().acceptValhalla()) {
        return false;
    }
    mirrorPolicyToGlobals();
    return true;
}

bool policyDeclineValhalla() {
    if (!policy().declineValhalla()) {
        return false;
    }
    mirrorPolicyToGlobals();
    return true;
}

bool policyEnterBlueTeam() {
    if (!policy().enterBlueTeam()) {
        return false;
    }
    mirrorPolicyToGlobals();
    return true;
}

bool policyLeaveBlueTeam(bool acknowledgedRisk) {
    if (!policy().leaveBlueTeam(acknowledgedRisk)) {
        return false;
    }
    mirrorPolicyToGlobals();
    return true;
}

'''

# ── 4. forward declarations ────────────────────────────────────────────────
FWD_OLD = '''bool isOffensiveAllowed();
void showBlueTeamBlockedScreen();
bool showDisclaimerScreen();
void recursiveDeleteSD(File dir);
void activateValhalla();
'''
FWD_NEW = '''bool requestOffensiveAccess();
bool isOffensiveAllowed();
void showBlueTeamBlockedScreen();
bool showDisclaimerScreen();
void showPolicyPanel();
bool confirmLeaveBlueTeam();
void recursiveDeleteSD(File dir);
void scorchedEarthWipe();
'''

# ── 5. settings submenu 10 -> 11 items ─────────────────────────────────────
SETTINGS_OLD = '''// Settings Submenu - 10 items
const int settings_NUM_SUBMENU_ITEMS = 10;
const char *settings_submenu_items[settings_NUM_SUBMENU_ITEMS] = {
    "Brightness",
    "Screen Timeout",
    "Swap Colors",
    "Invert Display",
    "Color Mode",
    "Rotation",
    "Device Info",
    "Set PIN",
    "CC1101 Module",
    "Back to Main Menu"
};

const unsigned char *settings_submenu_icons[settings_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,
    bitmap_icon_eye2,
    bitmap_icon_led,
    bitmap_icon_led,
    bitmap_icon_led,
    bitmap_icon_follow,
    bitmap_icon_stat,
    bitmap_icon_eye2,
    bitmap_icon_antenna,
    bitmap_icon_go_back
};
'''
SETTINGS_NEW = '''// Settings Submenu - 11 items
const int settings_NUM_SUBMENU_ITEMS = 11;
const char *settings_submenu_items[settings_NUM_SUBMENU_ITEMS] = {
    "Brightness",
    "Screen Timeout",
    "Swap Colors",
    "Invert Display",
    "Color Mode",
    "Rotation",
    "Device Info",
    "Set PIN",
    "CC1101 Module",
    "Device Policy",
    "Back to Main Menu"
};

const unsigned char *settings_submenu_icons[settings_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,
    bitmap_icon_eye2,
    bitmap_icon_led,
    bitmap_icon_led,
    bitmap_icon_led,
    bitmap_icon_follow,
    bitmap_icon_stat,
    bitmap_icon_eye2,
    bitmap_icon_antenna,
    bitmap_icon_eye2,
    bitmap_icon_go_back
};
'''

# ── 6. tools submenu 6 -> 7 items ──────────────────────────────────────────
TOOLS_OLD = '''// Tools Submenu - 6 items
const int tools_NUM_SUBMENU_ITEMS = 6;
const char *tools_submenu_items[tools_NUM_SUBMENU_ITEMS] = {
    "Serial Monitor",
    "Update Firmware",
    "Touch Calibrate",
    "GPS",
    "Radio Test",
    "Back to Main Menu"
};

const unsigned char *tools_submenu_icons[tools_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_bash,
    bitmap_icon_follow,
    bitmap_icon_stat,
    bitmap_icon_antenna,
    bitmap_icon_signal,
    bitmap_icon_go_back
};
'''
TOOLS_NEW = '''// Tools Submenu - 7 items
const int tools_NUM_SUBMENU_ITEMS = 7;
const char *tools_submenu_items[tools_NUM_SUBMENU_ITEMS] = {
    "Serial Monitor",
    "Update Firmware",
    "Touch Calibrate",
    "GPS",
    "Radio Test",
    "Wipe SD Card",
    "Back to Main Menu"
};

const unsigned char *tools_submenu_icons[tools_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_bash,
    bitmap_icon_follow,
    bitmap_icon_stat,
    bitmap_icon_antenna,
    bitmap_icon_signal,
    bitmap_icon_nuke,
    bitmap_icon_go_back
};
'''

# ── 7. settings touch handler: back index + Device Policy row ──────────────
SET_BACK_OLD = '''            if (current_submenu_index == 9) { // Back
                returnToMainMenu();
                return;
            }
'''
SET_BACK_NEW = '''            if (current_submenu_index == 10) { // Back
                returnToMainMenu();
                return;
            }
'''

SET_CASE_OLD = '''                case 8: // CC1101 Module Type
                    cc1101ModuleLoop();
                    break;
            }
'''
SET_CASE_NEW = '''                case 8: // CC1101 Module Type
                    cc1101ModuleLoop();
                    break;
                case 9: // Device Policy — the only place policy state changes
                    showPolicyPanel();
                    break;
            }
'''

# ── 8. tools touch handler: back index + wipe row ──────────────────────────
TOOLS_BACK_OLD = '''            if (current_submenu_index == 5) { // Back
                returnToMainMenu();
                return;
            }
'''
TOOLS_BACK_NEW = '''            if (current_submenu_index == 6) { // Back
                returnToMainMenu();
                return;
            }
'''

TOOLS_WIPE_OLD = '''            // Firmware Update - flash .bin from SD card
            if (current_submenu_index == 1) {
                firmwareUpdateScreen();
                returnToSubmenu();
                break;
            }
'''
TOOLS_WIPE_NEW = '''            // Firmware Update - flash .bin from SD card
            if (current_submenu_index == 1) {
                firmwareUpdateScreen();
                returnToSubmenu();
                break;
            }

            // Wipe SD Card - destructive, deliberately separate from the
            // Device Policy panel so the policy banner never erases data.
            if (current_submenu_index == 5) {
                scorchedEarthWipe();
                returnToSubmenu();
                break;
            }
'''

# ── 9. Blue Team block screen: stop promising the disclaimer unlock ────────
BT_HINT_OLD = '''    drawCenteredText(SCALE_Y(220), "Tap offensive tool to unlock", HALEHOUND_GUNMETAL, 1);
    drawCenteredText(SCALE_Y(235), "via disclaimer acceptance", HALEHOUND_GUNMETAL, 1);
'''
BT_HINT_NEW = '''    drawCenteredText(SCALE_Y(220), "Unlock: Settings > Device Policy", HALEHOUND_GUNMETAL, 1);
    drawCenteredText(SCALE_Y(235), "Blue Team stays until you leave it", HALEHOUND_GUNMETAL, 1);
'''

# ── 10. disclaimer screen: BOOT / ACCEPT / DECLINE ─────────────────────────
BOOT_OLD = '''        // BOOT button = decline
        if (buttonPressed(BTN_BOOT)) {
            delay(200);
            disclaimer_accepted = false;
            blue_team_mode = true;
            saveSettings();
            return false;
        }
'''
BOOT_NEW = '''        // BOOT button = decline -> persistent Blue Team
        if (buttonPressed(BTN_BOOT)) {
            delay(200);
            policyDeclineValhalla();
            return false;
        }
'''

ACCEPT_OLD = '''                disclaimer_accepted = true;
                if (blue_team_mode) {
                    blue_team_mode = false;
                }
                saveSettings();
                return true;
'''
ACCEPT_NEW = '''                // PolicyService refuses this while Blue Team is active — that
                // refusal is what makes Blue Team sticky. It can only succeed
                // from the Undecided state.
                if (policyAcceptValhalla()) {
                    return true;
                }
                showBlueTeamBlockedScreen();
                return false;
'''

DECLINE_OLD = '''            // DECLINE button
            if (tx >= dDeclineX && tx <= dDeclineX + dBtnW && ty >= dBtnY && ty <= dBtnY + dBtnH) {
                delay(200);
                disclaimer_accepted = false;
                blue_team_mode = true;
                saveSettings();
                return false;
            }
'''
DECLINE_NEW = '''            // DECLINE button -> persistent Blue Team
            if (tx >= dDeclineX && tx <= dDeclineX + dBtnW && ty >= dBtnY && ty <= dBtnY + dBtnH) {
                delay(200);
                policyDeclineValhalla();
                return false;
            }
'''

# ── 11. isOffensiveAllowed -> single gate ──────────────────────────────────
GATE_FN_OLD = '''// Check if offensive functions are allowed
bool isOffensiveAllowed() {
    return PolicyService::legacyAllowsOffensive(disclaimer_accepted,
                                                blue_team_mode);
}
'''
GATE_FN_NEW = '''// ─── Single point of authorization for every offensive entry point ──────
// Returns true only when the caller may proceed, and owns the screen shown
// when it may not. Every menu, deep link and feature-to-feature handoff must
// call this instead of re-implementing the gate.
bool requestOffensiveAccess() {
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

// Kept for read-only probes such as the home-screen mode banner.
bool isOffensiveAllowed() {
    return policy().offensiveAllowed();
}
'''

# ── 12. policy panel inserted before the SD wipe helper ────────────────────
POLICY_PANEL = '''// ─── Device Policy Panel ────────────────────────────────────────────────
// The single place the user changes policy state. Nothing here destroys data;
// the destructive SD wipe is a separate named Tools action.
bool confirmLeaveBlueTeam() {
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(0, 0, skull_bg_bitmap, SKULL_BG_WIDTH, SKULL_BG_HEIGHT, 0x1841);
    tft.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, HALEHOUND_VIOLET);
    tft.drawRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, HALEHOUND_MAGENTA);

    drawGlitchTitle(SCALE_Y(70), "LEAVE");
    drawGlitchStatus(SCALE_Y(95), "BLUE TEAM?", HALEHOUND_HOTPINK);

    drawCenteredText(SCALE_Y(130), "Offensive tools will be unlocked.", HALEHOUND_MAGENTA, 1);
    drawCenteredText(SCALE_Y(148), "You accept full personal liability", HALEHOUND_MAGENTA, 1);
    drawCenteredText(SCALE_Y(164), "for their use.", HALEHOUND_MAGENTA, 1);

    const int btnW = SCALE_W(95);
    const int btnH = SCALE_H(34);
    const int btnY = SCALE_Y(215);
    const int okX = SCALE_X(15);
    const int noX = SCREEN_WIDTH - btnW - SCALE_X(15);

    tft.fillRoundRect(okX, btnY, btnW, btnH, 5, HALEHOUND_DARK);
    tft.drawRoundRect(okX, btnY, btnW, btnH, 5, HALEHOUND_HOTPINK);
    tft.setTextSize(1);
    tft.setTextColor(HALEHOUND_HOTPINK);
    tft.setCursor(okX + 14, btnY + (btnH - 8) / 2);
    tft.print("I ACCEPT");

    tft.fillRoundRect(noX, btnY, btnW, btnH, 5, HALEHOUND_DARK);
    tft.drawRoundRect(noX, btnY, btnW, btnH, 5, HALEHOUND_GUNMETAL);
    tft.setTextColor(HALEHOUND_GUNMETAL);
    tft.setCursor(noX + 20, btnY + (btnH - 8) / 2);
    tft.print("CANCEL");

    while (true) {
        touchButtonsUpdate();
        if (isInoBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
            waitForTouchRelease();
            return false;
        }
        if (isTouchInArea(okX, btnY, btnW, btnH)) {
            waitForTouchRelease();
            return true;
        }
        if (isTouchInArea(noX, btnY, btnW, btnH)) {
            waitForTouchRelease();
            return false;
        }
        delay(30);
    }
}

void showPolicyPanel() {
    const int rowH = SCALE_H(38);
    const int rowGap = SCALE_H(6);
    const int firstRowY = SCALE_Y(100);
    const int x = SCALE_X(14);
    const int w = SCREEN_WIDTH - 2 * SCALE_X(14);

    while (true) {
        const PolicyMode mode = policy().mode();

        const char* labels[3] = {nullptr, nullptr, nullptr};
        int actionCount = 1;
        if (mode == PolicyMode::Undecided) {
            labels[0] = "ACCEPT DISCLAIMER";
            labels[1] = "STAY IN BLUE TEAM";
            actionCount = 2;
        } else if (mode == PolicyMode::BlueTeam) {
            labels[0] = "LEAVE BLUE TEAM";
        } else {
            labels[0] = "ENTER BLUE TEAM";
        }
        const int rows = actionCount + 1;  // + BACK

        const char* modeText = "MODE: DISCLAIMER PENDING";
        uint16_t modeColor = HALEHOUND_GUNMETAL;
        if (mode == PolicyMode::ValhallaAccepted) {
            modeText = "MODE: VALHALLA";
            modeColor = HALEHOUND_MAGENTA;
        } else if (mode == PolicyMode::BlueTeam) {
            modeText = "MODE: BLUE TEAM";
            modeColor = VALHALLA_BLUE;
        }

        // ── draw ──
        tft.fillScreen(TFT_BLACK);
        tft.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, HALEHOUND_VIOLET);
        tft.drawRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, HALEHOUND_MAGENTA);
        drawGlitchTitle(SCALE_Y(24), "DEVICE POLICY");
        drawGlitchStatus(SCALE_Y(58), modeText, modeColor);

        for (int i = 0; i < rows; i++) {
            const int y = firstRowY + i * (rowH + rowGap);
            const bool isBack = (i == actionCount);
            tft.fillRoundRect(x, y, w, rowH, 5, HALEHOUND_DARK);
            tft.drawRoundRect(x, y, w, rowH, 5,
                              isBack ? HALEHOUND_GUNMETAL : modeColor);
            tft.setTextSize(1);
            tft.setTextColor(isBack ? HALEHOUND_GUNMETAL : TFT_WHITE);
            tft.setCursor(x + 10, y + (rowH - 8) / 2);
            tft.print(isBack ? "BACK" : labels[i]);
        }

        tft.setTextSize(1);
        tft.setTextColor(HALEHOUND_GUNMETAL, TFT_BLACK);
        tft.setCursor(x, firstRowY + rows * (rowH + rowGap) + SCALE_H(4));
        tft.print("Offensive use may violate local law.");

        // ── touch ──
        bool redraw = false;
        while (!redraw) {
            touchButtonsUpdate();
            if (isInoBackTapped() || buttonPressed(BTN_BACK) || buttonPressed(BTN_BOOT)) {
                waitForTouchRelease();
                return;
            }
            for (int i = 0; i < rows; i++) {
                const int y = firstRowY + i * (rowH + rowGap);
                if (!isTouchInArea(x, y, w, rowH)) {
                    continue;
                }
                waitForTouchRelease();
                if (i == actionCount) {
                    return;  // BACK
                }
                if (mode == PolicyMode::Undecided && i == 0) {
                    if (!policyAcceptValhalla()) {
                        showBlueTeamBlockedScreen();
                    }
                } else if (mode == PolicyMode::Undecided && i == 1) {
                    policyDeclineValhalla();
                } else if (mode == PolicyMode::BlueTeam) {
                    if (confirmLeaveBlueTeam()) {
                        policyLeaveBlueTeam(true);
                    }
                } else {
                    policyEnterBlueTeam();
                }
                redraw = true;
                break;
            }
            delay(30);
        }
    }
}

'''

PANEL_ANCHOR = '''// ─── SD Card Recursive Wipe ─────────────────────────────────────────────
'''

# ── 13. scorched earth: explicit naming, no longer reachable from the banner ─
SE_HEADER_OLD = '''// ─── VALHALLA Protocol Activation ───────────────────────────────────────
void activateValhalla() {
'''
SE_HEADER_NEW = '''// ─── Scorched Earth: destructive SD wipe ────────────────────────────────
// Renamed out of the VALHALLA policy flow. It destroys data, so it is a named
// Tools action with its own hold-to-confirm — never a side effect of tapping
// the policy banner.
void scorchedEarthWipe() {
'''

SE_TITLE_OLD = '''    drawGlitchTitle(SCALE_Y(175), "VALHALLA");
    drawGlitchStatus(SCALE_Y(195), "PROTOCOL", HALEHOUND_VIOLET);
'''
SE_TITLE_NEW = '''    drawGlitchTitle(SCALE_Y(175), "WIPE SD");
    drawGlitchStatus(SCALE_Y(195), "SCORCHED EARTH", HALEHOUND_VIOLET);
'''

SE_ASK_OLD = '''    drawCenteredText(SCALE_Y(215), "ACTIVATE SCORCHED EARTH?", HALEHOUND_HOTPINK, 1);
'''
SE_ASK_NEW = '''    drawCenteredText(SCALE_Y(215), "ERASE SD AND LOCK TOOLS?", HALEHOUND_HOTPINK, 1);
'''

# ── 14. banner tap opens the policy panel, not the wipe ────────────────────
BANNER_OLD = '''                delay(150);
                activateValhalla();
                // If cancelled, redraw menu
                menu_initialized = false;
                displayMenu();
                delay(200);
                return;
'''
BANNER_NEW = '''                delay(150);
                showPolicyPanel();
                // Redraw menu when the panel closes
                menu_initialized = false;
                displayMenu();
                delay(200);
                return;
'''

# ── 15. boot: seed the policy store right after the legacy settings load ───
BOOTSTRAP_OLD = '''    // Load settings from EEPROM (brightness, timeout, color order, rotation, touch cal, color mode, PIN)
    loadSettings();
'''
BOOTSTRAP_NEW = '''    // Load settings from EEPROM (brightness, timeout, color order, rotation, touch cal, color mode, PIN)
    loadSettings();

    // Seed / refresh the policy store. After this the policy region is the
    // authority and the legacy disclaimer/blue-team bytes are mirrors.
    policyBootstrap();
'''

# ── 16. the 21 duplicated gates ────────────────────────────────────────────
GATE_RE = re.compile(
    r"([ \t]*)if \(!isOffensiveAllowed\(\)\) \{(\r?\n)"
    r"[ \t]*if \(blue_team_mode\) \{ showBlueTeamBlockedScreen\(\); if \(!showDisclaimerScreen\(\)\) break; \}\r?\n"
    r"[ \t]*else if \(!showDisclaimerScreen\(\)\) break;\r?\n"
    r"[ \t]*if \(!isOffensiveAllowed\(\)\) break;\r?\n"
    r"[ \t]*\}"
)
EXPECTED_GATES = 21


def main() -> None:
    dry = "--dry-run" in sys.argv
    with open(TARGET, "r", encoding="utf-8", newline="") as fh:
        text = fh.read()

    p = Patcher(text)

    p.insert_after_line("includes", '#include "services/MenuLayout.h"\n'.rstrip("\n"), INCLUDES)
    p.replace_line("globals", GLOBALS_OLD.rstrip("\n"), GLOBALS_NEW.rstrip("\n"))
    p.insert_after_line(
        "policy-store",
        "bool cc1101_pa_module = false;      // E07 PA module active (persisted in EEPROM)",
        POLICY_BLOCK,
    )
    p.replace_line("fwd-decls", FWD_OLD.rstrip("\n"), FWD_NEW.rstrip("\n"))
    p.replace_line("settings-menu", SETTINGS_OLD.rstrip("\n"), SETTINGS_NEW.rstrip("\n"))
    p.replace_line("tools-menu", TOOLS_OLD.rstrip("\n"), TOOLS_NEW.rstrip("\n"))
    p.sub_first_after(
        "settings-back",
        "void handleSettingsSubmenuTouch() {",
        SET_BACK_OLD.rstrip("\n"),
        SET_BACK_NEW.rstrip("\n"),
    )
    p.replace_line("settings-case", SET_CASE_OLD.rstrip("\n"), SET_CASE_NEW.rstrip("\n"))
    p.sub_first_after(
        "tools-back",
        "void handleToolsSubmenuTouch() {",
        TOOLS_BACK_OLD.rstrip("\n"),
        TOOLS_BACK_NEW.rstrip("\n"),
    )
    p.replace_line("tools-wipe", TOOLS_WIPE_OLD.rstrip("\n"), TOOLS_WIPE_NEW.rstrip("\n"))
    p.replace_line("blue-team-hint", BT_HINT_OLD.rstrip("\n"), BT_HINT_NEW.rstrip("\n"))
    p.replace_line("disclaimer-boot", BOOT_OLD.rstrip("\n"), BOOT_NEW.rstrip("\n"))
    p.replace_line("disclaimer-accept", ACCEPT_OLD.rstrip("\n"), ACCEPT_NEW.rstrip("\n"))
    p.replace_line("disclaimer-decline", DECLINE_OLD.rstrip("\n"), DECLINE_NEW.rstrip("\n"))
    p.replace_line("gate-function", GATE_FN_OLD.rstrip("\n"), GATE_FN_NEW.rstrip("\n"))
    # insert the panel *before* the SD wipe header
    anchor = PANEL_ANCHOR.rstrip("\n")
    if p.text.count(anchor) != 1:
        raise SystemExit("[FAIL] policy-panel: SD wipe header not unique")
    idx = p.text.find(anchor)
    eol = eol_of(p.text[idx:idx + len(anchor) + 2])
    p.text = p.text[:idx] + newline_block(POLICY_PANEL + "\n", eol) + p.text[idx:]
    p.log.append("  ok  policy-panel  x1")
    p.replace_line("scorched-header", SE_HEADER_OLD.rstrip("\n"), SE_HEADER_NEW.rstrip("\n"))
    p.replace_line("scorched-title", SE_TITLE_OLD.rstrip("\n"), SE_TITLE_NEW.rstrip("\n"))
    p.replace_line("scorched-ask", SE_ASK_OLD.rstrip("\n"), SE_ASK_NEW.rstrip("\n"))
    p.replace_line("banner-tap", BANNER_OLD.rstrip("\n"), BANNER_NEW.rstrip("\n"))
    p.replace_line("policy-bootstrap", BOOTSTRAP_OLD.rstrip("\n"), BOOTSTRAP_NEW.rstrip("\n"))

    # gates
    matches = list(GATE_RE.finditer(p.text))
    if len(matches) != EXPECTED_GATES:
        raise SystemExit(f"[FAIL] gates: expected {EXPECTED_GATES}, found {len(matches)}")
    for m in reversed(matches):
        eol = "\r\n" if m.group(2) == "\r\n" else "\n"
        p.text = (
            p.text[: m.start()]
            + f"{m.group(1)}if (!requestOffensiveAccess()) break;"
            + eol
            + p.text[m.end():]
        )
    p.log.append(f"  ok  gates  x{EXPECTED_GATES}")

    for line in p.log:
        print(line)

    if dry:
        print("\n[dry-run] nothing written")
        return
    with open(TARGET, "w", encoding="utf-8", newline="") as fh:
        fh.write(p.text)
    print("\n[ok] LegacyApp.inc patched")


if __name__ == "__main__":
    main()
