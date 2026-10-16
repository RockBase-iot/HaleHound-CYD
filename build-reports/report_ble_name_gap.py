#!/usr/bin/env python3
"""Report the UUID gap between include/ble_database.h and GattNameTable.

Why
---
B4 ("BLE name-library unification") needs the *numbers* single-sourced. Before
moving anything, measure the two tables:

  * which 16-bit service UUIDs `ble_database.h` carries that `GattNameTable`
    does not (a delegation would silently lose them), and
  * what the Bluetooth SIG actually calls each of those UUIDs.

The SIG name is the only acceptable source for a new `GattNameTable` row --
`verify_gatt_names.py` rejects any assigned UUID whose name is not verbatim.

Usage:
    python build-reports/report_ble_name_gap.py
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIG_SERVICES = ROOT / "build-reports" / "sig" / "service_uuids.yaml"
SIG_MEMBERS = ROOT / "build-reports" / "sig" / "member_uuids.yaml"
LEGACY = ROOT / "include" / "ble_database.h"
CANON = ROOT / "src" / "features" / "ble" / "GattNameTable.cpp"


def sig_names(path: Path) -> dict[int, str]:
    table: dict[int, str] = {}
    uuid = None
    for line in path.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        m = re.match(r"-?\s*uuid:\s*0x([0-9A-Fa-f]+)\s*$", s)
        if m:
            uuid = int(m.group(1), 16)
            continue
        m = re.match(r"name:\s*(.+?)\s*$", s)
        if m and uuid is not None:
            name = m.group(1)
            if len(name) >= 2 and name[0] == name[-1] and name[0] in "'\"":
                name = name[1:-1]
            table[uuid] = name
            uuid = None
    return table


def legacy_services(path: Path) -> list[tuple[int, str]]:
    """(uuid, display name) rows of bleServiceDB."""
    text = path.read_text(encoding="utf-8")
    block = re.search(r"bleServiceDB\[\] = \{(.*?)\n\};", text, re.S)
    if not block:
        sys.exit("report_ble_name_gap: cannot find bleServiceDB")
    # Rows reference `_sn_xxx` constants; resolve them from the file.
    names = dict(re.findall(r"static const char PROGMEM (_sn_\w+)\[\]\s*=\s*\"([^\"]*)\"", text))
    rows: list[tuple[int, str]] = []
    for m in re.finditer(r"\{\s*0x([0-9A-Fa-f]+)\s*,\s*(_sn_\w+)\s*\}", block.group(1)):
        rows.append((int(m.group(1), 16), names.get(m.group(2), "?")))
    return rows


def canonical_services(path: Path) -> dict[int, str]:
    text = path.read_text(encoding="utf-8")
    block = re.search(r"constexpr NamedUuid kServices\[\] = \{(.*?)\n\};", text, re.S)
    if not block:
        sys.exit("report_ble_name_gap: cannot find kServices")
    return {
        int(m.group(1), 16): m.group(2)
        for m in re.finditer(r"\{0x([0-9A-Fa-f]+),\s*\"([^\"]*)\"\}", block.group(1))
    }


def main() -> int:
    sig = sig_names(SIG_SERVICES)
    members = sig_names(SIG_MEMBERS) if SIG_MEMBERS.exists() else {}
    legacy = legacy_services(LEGACY)
    canon = canonical_services(CANON)

    print(f"SIG adopted 16-bit services  : {len(sig)}")
    print(f"SIG member UUIDs (0xFE/0xFD) : {len(members)}")
    print(f"GattNameTable kServices      : {len(canon)}")
    print(f"ble_database.h bleServiceDB  : {len(legacy)}")

    # 1. UUIDs the canonical table does not have yet.
    missing = [(u, n) for u, n in legacy if u not in canon]
    print(f"\n--- in ble_database.h, absent from GattNameTable ({len(missing)}) ---")
    for u, n in sorted(missing):
        assigned = sig.get(u)
        if assigned is not None:
            verdict = f"SIG      ={assigned!r}"
        elif u in members:
            verdict = f"(member) ={members[u]!r}"
        else:
            verdict = "*** NOT IN ANY SIG LIST ***"
        print(f"  0x{u:04X}  legacy={n!r:18}  {verdict}")

    # 2. Disagreements where both have the UUID: is the canonical one SIG-exact?
    print("\n--- shared UUIDs whose names differ ---")
    differ = 0
    for u, n in sorted(legacy):
        if u in canon and canon[u] != n:
            differ += 1
            print(f"  0x{u:04X}  legacy={n!r:18} canonical={canon[u]!r:28} SIG={sig.get(u)!r}")
    print(f"  ({differ} differ, which is expected: legacy names are 14-char display strings)")

    # 3. Any legacy UUID the SIG does not assign is a data bug worth naming.
    bad = [u for u, _ in legacy if u not in sig and u not in members]
    print(f"\n--- legacy UUIDs with no SIG assignment and no member record ({len(bad)}) ---")
    for u in sorted(bad):
        print(f"  0x{u:04X}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
