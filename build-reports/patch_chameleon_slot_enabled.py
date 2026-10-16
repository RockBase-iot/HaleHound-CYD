#!/usr/bin/env python3
"""Adds `enabled` to ChameleonSlot and keeps it in step in the session methods.

The eight-slot answer from GET_ENABLED_SLOTS (1023) is a per-slot pair of flags
with nowhere to land in the model, so the model grows one field rather than the
client throwing the answer away. Each replacement asserts it matched exactly
once, so a silent no-op cannot pass for a patch.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def patch(path: Path, pairs: list[tuple[str, str]]) -> int:
    text = path.read_text(encoding="utf-8")
    original = text
    for old, new in pairs:
        count = text.count(old)
        if count != 1:
            print(f"FAIL {path.name}: pattern matched {count} times, expected 1:\n---\n{old}\n---")
            return 1
        text = text.replace(old, new)
    if text == original:
        print(f"FAIL {path.name}: nothing changed")
        return 1
    path.write_text(text, encoding="utf-8")
    print(f"OK   {path.name}: {len(pairs)} replacement(s)")
    return 0


HEADER = ROOT / "include" / "features" / "rfid" / "ChameleonUltraModel.h"
HEADER_PAIRS = [
    (
        """struct ChameleonSlot {
    uint8_t index = 0;
    ChameleonSlotKind kind = ChameleonSlotKind::Empty;
    std::string nickname;
    bool loaded = false;
};""",
        """struct ChameleonSlot {
    uint8_t index = 0;
    ChameleonSlotKind kind = ChameleonSlotKind::Empty;
    std::string nickname;
    // True when a card has been read or written into this slot. A loaded slot
    // the device is not currently emulating is still loaded, which is why this
    // and `enabled` are separate answers rather than one flag.
    bool loaded = false;
    // True when the slot is switched on for emulation. The device reports this
    // per frequency (enabled_hf / enabled_lf); a slot enabled for either counts
    // as enabled, since the panel asks "is this slot live", not "on which band".
    bool enabled = false;
};""",
    ),
]

CPP = ROOT / "src" / "features" / "rfid" / "ChameleonUltraModel.cpp"
CPP_PAIRS = [
    (
        """    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        slots_[index].index = index;
        slots_[index].kind = ChameleonSlotKind::Empty;
        slots_[index].nickname.clear();
        slots_[index].loaded = false;
    }""",
        """    for (uint8_t index = 0; index < kChameleonSlotCount; ++index) {
        slots_[index].index = index;
        slots_[index].kind = ChameleonSlotKind::Empty;
        slots_[index].nickname.clear();
        slots_[index].loaded = false;
        slots_[index].enabled = false;
    }""",
    ),
    (
        """    slots_[index].kind = kind;
    slots_[index].nickname = nickname;
    slots_[index].loaded = kind != ChameleonSlotKind::Empty;
    return true;""",
        """    slots_[index].kind = kind;
    slots_[index].nickname = nickname;
    slots_[index].loaded = kind != ChameleonSlotKind::Empty;
    slots_[index].enabled = kind != ChameleonSlotKind::Empty;
    return true;""",
    ),
    (
        """    slots_[index].kind = ChameleonSlotKind::Empty;
    slots_[index].nickname.clear();
    slots_[index].loaded = false;
    return true;""",
        """    slots_[index].kind = ChameleonSlotKind::Empty;
    slots_[index].nickname.clear();
    slots_[index].loaded = false;
    slots_[index].enabled = false;
    return true;""",
    ),
]


def main() -> int:
    if patch(HEADER, HEADER_PAIRS) != 0:
        return 1
    if patch(CPP, CPP_PAIRS) != 0:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
