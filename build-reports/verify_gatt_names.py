#!/usr/bin/env python3
"""Diff GattNameTable's UUID tables against the Bluetooth SIG assigned numbers.

Why this exists
---------------
`GattNameTable.cpp` is a hand-curated table. Hand-curated data drifts, and a
wrong name in a GATT dump is worse than no name because the dump is evidence.

The SIG publishes the assigned numbers as machine-readable YAML, so the tables
can be checked rather than trusted:

    bitbucket.org/bluetooth-SIG/public/assigned_numbers/uuids/service_uuids.yaml
    bitbucket.org/bluetooth-SIG/public/assigned_numbers/uuids/characteristic_uuids.yaml
    bitbucket.org/bluetooth-SIG/public/assigned_numbers/uuids/descriptors.yaml

Fetched into build-reports/sig/ (git-ignored) by:

    curl -sSL -o build-reports/sig/<name>.yaml \\
      https://bitbucket.org/bluetooth-SIG/public/raw/HEAD/assigned_numbers/uuids/<name>.yaml

Contract checked here
---------------------
  * Every UUID the SIG assigns must carry the SIG's name VERBATIM -- including
    the abbreviated ones. The SIG calls 0x1800 "GAP", not "Generic Access", and
    0x180F "Battery", not "Battery Service". Following the SIG rather than
    common usage is what lets this check have no allowlist.
  * A UUID the SIG does not assign must carry an explicit provenance tag in its
    name: "(vendor)" for the 0xFFxx duplicate range, "(member)" for a
    member-assigned UUID like Nordic's DFU service. No tag means no pass.
  * Anything else is a failure. There is deliberately no "known exceptions"
    list: an entry that cannot be verified must be deleted, not excused.

This script does not rewrite the source. Fixing a mismatch is a source change
that should be reviewable, so it goes through a patch script like every other
edit in this repository.

Usage:
    python build-reports/verify_gatt_names.py
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIG_DIR = ROOT / "build-reports" / "sig"
SOURCE = ROOT / "src" / "features" / "ble" / "GattNameTable.cpp"

YAML_FILES = {
    "kServices": "service_uuids.yaml",
    "kCharacteristics": "characteristic_uuids.yaml",
    "kDescriptors": "descriptors.yaml",
}

# A name for a UUID the SIG does not assign must say where it came from.
PROVENANCE_TAGS = ("(vendor)", "(member)")


def parse_sig_yaml(path: Path) -> dict[int, str]:
    """Return {uuid: name} from an assigned-numbers YAML."""
    if not path.exists():
        sys.exit(
            f"verify_gatt_names: missing {path}. Download it first -- see this "
            f"script's docstring for the curl line."
        )
    table: dict[int, str] = {}
    uuid = None
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        match = re.match(r"-?\s*uuid:\s*0x([0-9A-Fa-f]+)\s*$", stripped)
        if match:
            uuid = int(match.group(1), 16)
            continue
        match = re.match(r"name:\s*(.+?)\s*$", stripped)
        if match and uuid is not None:
            name = match.group(1)
            if len(name) >= 2 and name[0] == name[-1] and name[0] in "'\"":
                name = name[1:-1]
            table[uuid] = name
            uuid = None
    return table


def parse_source(text: str) -> list[tuple[str, int, str]]:
    """Return [(table, uuid, name)] for every row of every table."""
    rows = []
    for table in YAML_FILES:
        match = re.search(r"constexpr NamedUuid " + table + r"\[\] = \{(.*?)\n\};", text, re.S)
        if not match:
            sys.exit(f"verify_gatt_names: cannot find table {table} in {SOURCE.name}")
        for row in re.finditer(r"\{0x([0-9A-Fa-f]+),\s*\"([^\"]*)\"\}", match.group(1)):
            rows.append((table, int(row.group(1), 16), row.group(2)))
    return rows


def main() -> int:
    text = SOURCE.read_text(encoding="utf-8")
    rows = parse_source(text)
    sig = {table: parse_sig_yaml(SIG_DIR / name) for table, name in YAML_FILES.items()}

    mismatched: list[tuple[str, int, str, str]] = []
    untagged: list[tuple[str, int, str]] = []
    tagged = 0
    for table, uuid, name in rows:
        assigned = sig[table].get(uuid)
        if assigned is None:
            if any(tag in name for tag in PROVENANCE_TAGS):
                tagged += 1
            else:
                untagged.append((table, uuid, name))
        elif assigned != name:
            mismatched.append((table, uuid, name, assigned))

    total = len(rows)
    available = sum(len(v) for v in sig.values())
    print(f"verify_gatt_names: {total} table entries, {available} assigned numbers available")
    print(f"  assigned, name-exact : {total - tagged - len(untagged) - len(mismatched)}")
    print(f"  unassigned, tagged   : {tagged}")

    if mismatched:
        print(f"\n  NAME MISMATCHES ({len(mismatched)}) -- SIG name is authoritative:")
        for table, uuid, got, want in mismatched:
            print(f"    {table} 0x{uuid:04X}: table={got!r}  SIG={want!r}")

    if untagged:
        print(f"\n  UNASSIGNED WITHOUT A PROVENANCE TAG ({len(untagged)}):")
        for table, uuid, name in untagged:
            print(f"    {table} 0x{uuid:04X}: {name!r}")

    if mismatched or untagged:
        print("\nFAIL: unverifiable entries must be corrected or deleted, not excused.")
        return 1

    print("\nOK: every assigned UUID matches the SIG name verbatim; every other "
          "entry declares its provenance.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
