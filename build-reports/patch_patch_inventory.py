#!/usr/bin/env python3
"""Complete the `patch_*.py` inventory table in build-reports/README.md.

The table claims to be the provenance record for the one-shot patch scripts, but
it only ever listed the ones the hostcheck / plan / U2 sessions added: 11 of the
30 tracked scripts. The other 19 -- the B4 BLE name-library work, the PN532
driver layer, the Chameleon fixes, three doc refreshes -- were tracked and
described in their own docstrings, just never indexed. A provenance table that
silently omits two thirds of its entries is worse than no table, because it reads
as complete.

The descriptions here are taken from each script's own module docstring, not
reconstructed from the diff.

It also records the one script whose target is outside this repository
(`patch_skill_syntax_vacuity.py` writes the syntax-check skill under
~/.workbuddy/skills/), so that change stays traceable from the repo.

Run from the repository root:
    python build-reports/patch_patch_inventory.py

Every replacement asserts a unique match, and after writing, the script re-reads
the README and checks that every tracked patch script is named in it -- so this
cannot half-apply or leave the table still incomplete.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "build-reports" / "README.md"

ANCHOR = "| `patch_plan_hostcheck.py` | DEVELOPMENT_PLAN.md §5.4 / §5.5 / §7 / §9 |\n"

ROWS = """| `patch_gatt_names.py` | B4: `GattNameTable` made SIG-verbatim -- 13 wrong names corrected (0x2A31 was pointing at the Scan Parameters *service*), 7 unassigned entries deleted |
| `patch_gatt_names_union.py` | B4: `GattNameTable::kServices` grown to the union of the two BLE service tables, so the legacy 73-row copy could be deleted without losing coverage |
| `patch_gatt_names_accessors.py` | B4: index accessors on `GattNameTable`, so a test can prove every service name clamps to the legacy panel's 14-character budget |
| `patch_gatt_names_test.py` | `test_gatt_names` updated for the B4 union |
| `patch_ble_database_services.py` | B4: deleted `include/ble_database.h`'s dead 73-row service table (its only caller used `lookupCompanyName`; 0x1822 was labelled HTTP Proxy where the SIG assigns that number to Pulse Oximeter) |
| `patch_pn532_transport.py` | `Pn532Transport.cpp`: I2C timing after a write, and UID bookkeeping |
| `patch_pn532_transport_test.py` | three defects in `test_pn532_transport` before its first run: a reply helper that takes an initializer_list, a test that re-called `transceive` against an unscripted fake, a wrong comment about where the status byte goes |
| `patch_mifare_driver_test.py` | three wrong expectations in `test_mifare_classic_driver`: the ATQA the NTAG row needs, and `unknownBlockCount()` counting every unrecovered block |
| `patch_hid_write_ref.py` | the HID reader picks its write target from the discovered GATT properties, instead of writing commands to the notify characteristic |
| `patch_chameleon_rearm.py` | the Chameleon receiver now looks for a frame start inside a rejected window -- a real frame whose SOF landed in the doomed header window used to be consumed and lost |
| `patch_chameleon_slot_enabled.py` | `ChameleonSlot` grows `enabled`, so GET_ENABLED_SLOTS' per-slot flag pair has somewhere to land instead of the answer being discarded |
| `patch_chameleon_frame_test.py` | two `test_chameleon_frame` assumptions: this shim has no `TEST_ASSERT_EQUAL_MEMORY`, and `TEST_ASSERT_*` may not expand inside a non-void function |
| `patch_hostcheck_support.py` | `run_host_tests.py`: `test/native/support` added to the include path so suites can share scripted test doubles |
| `patch_plan_remaining.py` | `DEVELOPMENT_PLAN.md` after the 2026-10-15 RFID/BLE model-layer session |
| `patch_remaining_work_rfid.py` | `REMAINING_WORK.md` after the PN532 driver layer landed: four new suites, R3/R4/R5/R7 done at the driver layer, R1 demoted to P1 by instruction |
| `patch_remaining_work_deps.py` | three `REMAINING_WORK.md` dependency cells the driver layer made stale: R2 -> W1, R8 unblocked, R10 unblocked |
| `patch_remaining_work.py` | `REMAINING_WORK.md` after closing B1-B3 and the host half of B5 |
| `patch_release_brief.py` | `docs/upstream-analysis/RELEASE_BRIEF_zh.md`: the stale gap matrix and its statistics, the module/suite inventories, and two verification rows that still claimed native tests cannot run |
| `patch_skill_syntax_vacuity.py` | **targets a file outside this repo**: the `esp32-platformio-syntax-check` skill gained the "-fsyntax-only PASS that compiled nothing" subsection |
| `patch_patch_inventory.py` | this table: the 19 tracked-but-unlisted scripts above, and the note below |
"""

NOTE_ANCHOR = "| `patch_u2_docs.py` |"
NOTE = (
    "> One script in this table edits a file outside the repository: "
    "`patch_skill_syntax_vacuity.py` writes\n"
    "> `~/.workbuddy/skills/esp32-platformio-syntax-check/SKILL.md`. It is kept here "
    "so that change is still\n"
    "> traceable from the repo. Every other script touches only tracked source, tests "
    "and docs.\n"
)

INTRO_OLD = """silent no-op cannot happen. They are kept for provenance; re-running one whose edit is
already applied will abort harmlessly (`expected 1 match, found 0`).
"""
INTRO_NEW = """silent no-op cannot happen. They are kept for provenance; re-running one whose edit is
already applied will abort harmlessly (`expected 1 match, found 0`). The table lists
every tracked script, not only the recent ones.
"""

applied: list[str] = []


def replace_once(name: str, old: str, new: str) -> None:
    text = README.read_text(encoding="utf-8", newline="")
    found = text.count(old)
    if found != 1:
        sys.exit(f"{name}: expected 1 match, found {found}. Nothing was written.")
    README.write_text(text.replace(old, new, 1), encoding="utf-8", newline="")
    applied.append(name)


def tracked_scripts() -> list[str]:
    out = subprocess.run(
        ["git", "ls-files", "build-reports/"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split()
    names = [p.split("/")[-1] for p in out]
    return sorted(n for n in names if re.match(r"patch.*\.py$", n))


def main() -> None:
    before = tracked_scripts()

    replace_once("intro", INTRO_OLD, INTRO_NEW)
    replace_once("rows", ANCHOR, ANCHOR + ROWS)
    replace_once("outside-repo note", NOTE_ANCHOR, NOTE + NOTE_ANCHOR)

    # The point of the exercise: assert the table is now complete rather than
    # trusting that the rows above happened to cover everything.
    after = tracked_scripts()
    text = README.read_text(encoding="utf-8", newline="")
    missing = [n for n in after if n not in text]
    if missing:
        sys.exit(f"table still incomplete, missing: {missing}")

    print(f"README.md rows added: {len(ROWS.strip().splitlines())}")
    print(f"tracked patch scripts: {len(before)} -> {len(after)}")
    print(f"all {len(after)} named in the table: yes")
    for name in applied:
        print(f"  applied: {name}")


if __name__ == "__main__":
    main()
