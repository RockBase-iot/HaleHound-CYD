#!/usr/bin/env python3
"""Grow GattNameTable::kServices to the union of the two BLE service tables.

B4 ("BLE name-library unification"): the *numbers* must live in exactly one
place -- `GattNameTable`. `include/ble_database.h`'s service section (73 rows)
and `GattNameTable`'s (44 rows) overlap by 42, so before the legacy copy can be
deleted, `GattNameTable` has to cover everything the legacy table carried or
the deletion silently loses coverage.

Names come from the SIG machine-readable lists, never from the legacy table --
`verify_gatt_names.py` rejects any assigned UUID whose name is not verbatim:
  * adopted services -> assigned_numbers/uuids/service_uuids.yaml
  * member UUIDs     -> assigned_numbers/uuids/member_uuids.yaml (company only)

For a member UUID the SIG list gives the *company*, not the service's function;
those five rows therefore carry a `(member)` provenance tag and a commonly-used
function name, and are recorded as PARTIALLY VERIFIED in the evidence doc.

Sorted order matters: `lookup()` is a linear scan, so an out-of-order row is
unreachable rather than a crash. The script asserts strict ascending before it
writes anything.

Usage:
    python build-reports/patch_gatt_names_union.py [--dry-run]
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIG_DIR = ROOT / "build-reports" / "sig"
SOURCE = ROOT / "src" / "features" / "ble" / "GattNameTable.cpp"

# 0xFE/0xFD member UUIDs whose numbers the SIG assigns to a company. The number
# and the company are verified; the function name is community usage, hence the
# `(member)` tag and the PARTIALLY VERIFIED grade in gatt-names.md.
MEMBER_ROWS: dict[int, tuple[str, str]] = {
    0xFD0D: ("Blecon service", "Blecon Ltd"),
    0xFE0F: ("Philips Hue service", "Signify Netherlands B.V."),
    0xFE2C: ("Fast Pair service", "Google LLC"),
    0xFEAA: ("Eddystone beacon", "Google LLC"),
    0xFEBB: ("Adafruit service", "adafruit industries"),
}


def parse_yaml(path: Path) -> dict[int, str]:
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


def legacy_service_uuids() -> list[int]:
    """Every 16-bit service UUID include/ble_database.h currently carries."""
    text = (ROOT / "include" / "ble_database.h").read_text(encoding="utf-8")
    block = re.search(r"bleServiceDB\[\] = \{(.*?)\n\};", text, re.S)
    if not block:
        sys.exit("patch_gatt_names_union: cannot find bleServiceDB")
    return sorted(int(m.group(1), 16)
                  for m in re.finditer(r"\{\s*0x([0-9A-Fa-f]+)\s*,", block.group(1)))


def main() -> int:
    dry = "--dry-run" in sys.argv
    text = SOURCE.read_text(encoding="utf-8")

    block = re.search(r"(constexpr NamedUuid kServices\[\] = \{)(.*?)(\n\};)", text, re.S)
    if not block:
        sys.exit("patch_gatt_names_union: cannot find kServices")

    current = {
        int(m.group(1), 16): m.group(2)
        for m in re.finditer(r"\{0x([0-9A-Fa-f]+),\s*\"([^\"]*)\"\}", block.group(2))
    }

    adopted = parse_yaml(SIG_DIR / "service_uuids.yaml")
    members = parse_yaml(SIG_DIR / "member_uuids.yaml")
    legacy = legacy_service_uuids()

    added: dict[int, str] = {}
    grades: list[tuple[int, str, str]] = []

    for uuid in legacy:
        if uuid in current:
            continue
        if uuid in adopted:
            added[uuid] = adopted[uuid]
            grades.append((uuid, "VERIFIED", "adopted service, SIG name verbatim"))
        elif uuid in MEMBER_ROWS:
            name, company = MEMBER_ROWS[uuid]
            if uuid not in members:
                sys.exit(f"patch_gatt_names_union: 0x{uuid:04X} not in member_uuids.yaml")
            added[uuid] = f"{name} (member)"
            grades.append((uuid, "PARTIAL", f"member UUID, company = {company}"))
        else:
            sys.exit(f"patch_gatt_names_union: 0x{uuid:04X} in neither SIG list -- "
                     "cannot name it, and it must not be invented")

    merged = dict(current)
    merged.update(added)
    ordered = sorted(merged.items())

    if any(ordered[i][0] <= ordered[i - 1][0] for i in range(1, len(ordered))):
        sys.exit("patch_gatt_names_union: merged table is not strictly ascending")

    body = "".join(f'\n    {{0x{uuid:04X}, "{name}"}},' for uuid, name in ordered)
    replacement = f"{block.group(1)}{body}{block.group(3)}"

    count = text.count(block.group(0))
    if count != 1:
        sys.exit(f"patch_gatt_names_union: matched {count} blocks, expected 1")

    print(f"kServices {len(current)} -> {len(merged)}  (+{len(added)})")
    for uuid, grade, why in grades:
        print(f"  + 0x{uuid:04X}  {grade:8} {merged[uuid]!r}  ({why})")

    if dry:
        print("\n--dry-run: nothing written")
        return 0

    SOURCE.write_text(text.replace(block.group(0), replacement), encoding="utf-8")

    after = SOURCE.read_text(encoding="utf-8")
    verify = re.search(r"constexpr NamedUuid kServices\[\] = \{(.*?)\n\};", after, re.S)
    written = len(re.findall(r"\{0x[0-9A-Fa-f]+,\s*\"", verify.group(1)))
    if written != len(merged):
        sys.exit(f"patch_gatt_names_union: wrote {written} rows, expected {len(merged)}")
    print(f"\nwrote {written} kServices rows to {SOURCE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
