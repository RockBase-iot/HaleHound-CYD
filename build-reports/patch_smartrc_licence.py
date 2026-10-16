#!/usr/bin/env python3
"""Teach the licence audit that a bespoke notice can still be a grant.  (C10)

The audit had two buckets for a library that ships a licence file which no
matcher recognises: `unidentified`.  That was right for the first reading --
the file is not a named licence -- but the conclusion it implied was too
strong.  The full text of `SmartRC-CC1101-Driver-Lib@2.5.7/LICENSE.txt` reads:

    cc1101 Driver for RC Switch. Mod by Little Satan. With permission to
    modify and publish Wilson Shen (ELECHOUSE).
    ...
    You can freely use, edit or distribute it with reference to the source.

That *is* a grant: use, edit and distribution are expressly allowed.  It is
also conditional -- "with reference to the source" -- and `library.json` /
`library.properties` carry no `license` field at all, so nothing else supplies
the missing scope.  A row saying "unidentified" sends a reader away to read the
file themselves; a row saying "bespoke-permissive, requires source reference"
tells them what to do.  Only the second one is a finding.

So SmartRC moves out of `unidentified` into a new `bespoke-permissive` class
whose SPDX expression is a `LicenseRef` -- the standard way to name a licence
that has no SPDX identifier -- and whose obligation (keep the source
reference) is printed as a NOTE rather than a VERDICT: it is a condition on
distribution, but it is a condition this repository already satisfies through
NOTICE.md and THIRD_PARTY_NOTICES.md, so it must not fail the gate.

Three edits in audit_licenses.py and three in collect_licenses.py.  Idempotent:
a pair whose replacement text is already present is reported as applied and
skipped, so re-running after a partial failure is safe.

Usage:  python build-reports/patch_smartrc_licence.py
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

AUDIT = "build-reports/audit_licenses.py"
COLLECT = "build-reports/collect_licenses.py"

EDITS: dict[str, list[tuple[str, str]]] = {}

# --------------------------------------------------------------------------
# audit_licenses.py
# --------------------------------------------------------------------------

EDITS[AUDIT] = [
    # 1. The bespoke class itself, right after COMPOSITE -- both are cases where
    #    the shipped text cannot be summarised by a single matched pattern.
    (
        r"""    for name, (spdx, kind, evidence) in COMPOSITE.items():
        if name in seen:
            seen[name].update(spdx=spdx, kind=kind, evidence=evidence)
""",
        r"""    for name, (spdx, kind, evidence) in COMPOSITE.items():
        if name in seen:
            seen[name].update(spdx=spdx, kind=kind, evidence=evidence)

    # Bespoke notices: no matcher names them, but the text does state a
    # permission.  SmartRC's LICENSE.txt allows use, editing and distribution
    # "with reference to the source" -- a real grant carrying a real
    # condition, written in the author's own words instead of SPDX vocabulary.
    # Reporting that as merely "unidentified" understates it in the direction
    # that costs the most: a reader concludes the grant is unknown when it is
    # in fact known and merely non-standard.  A LicenseRef is the SPDX way to
    # say "a licence, not in the list"; the condition is printed as a NOTE.
    BESPOKE = {
        "SmartRC-CC1101-Driver-Lib": (
            "LicenseRef-SmartRC-bespoke-permissive", "bespoke-permissive",
            "LICENSE.txt (bespoke grant: free use/edit/distribution, "
            "conditioned on referring to the source)"),
    }
    for name, (spdx, kind, evidence) in BESPOKE.items():
        if name in seen:
            seen[name].update(spdx=spdx, kind=kind, evidence=evidence)
""",
    ),
    # 2. A bucket for it, so it stops being counted as "unclear".
    (
        r"""    unclear = [e for e in libs if e["kind"] == "unidentified"]
    okay = [e for e in libs if e["kind"] == "permissive"]
""",
        r"""    unclear = [e for e in libs if e["kind"] == "unidentified"]
    bespoke = [e for e in libs if e["kind"] == "bespoke-permissive"]
    okay = [e for e in libs if e["kind"] == "permissive"]
""",
    ),
    # 3. Print it next to the other classes.
    (
        r"""    print(f"permissive      : {len(okay):2d}  {[e['name'] for e in okay]}")
""",
        r"""    print(f"permissive      : {len(okay):2d}  {[e['name'] for e in okay]}")
    print(f"bespoke grant   : {len(bespoke):2d}  "
          f"{[e['name'] + ' (' + e['evidence'] + ')' for e in bespoke]}")
""",
    ),
    # 4. The obligation, as a NOTE.  It must not enter `verdicts`, because a
    #    non-empty `verdicts` is what makes this script exit 1.
    (
        r"""    print()
    for verdict in verdicts:
        print("VERDICT:", verdict)
""",
        r"""    print()
    for verdict in verdicts:
        print("VERDICT:", verdict)

    # Conditions that do not block.  A bespoke grant that asks for attribution
    # is satisfied by carrying the source reference -- which NOTICE.md and
    # THIRD_PARTY_NOTICES.md do -- so it must not fail the gate, and must not
    # silently vanish either.  Anything that is a condition but not a blocker
    # belongs here rather than in `verdicts`.
    notes = []
    if bespoke:
        notes.append(
            f"{len(bespoke)} bespoke grant(s) permit use, modification and "
            f"distribution on the condition that the source is referred to: "
            f"{', '.join(e['name'] for e in bespoke)}.  NOTICE.md and "
            f"THIRD_PARTY_NOTICES.md carry that reference, so this does not "
            f"block distribution -- but it is a condition, not a courtesy.")
    for note in notes:
        print("NOTE:", note)
""",
    ),
    # 5. The exit-code comment now has to explain why two *kinds* of finding
    #    block, since C1 has to clear both.
    (
        r"""    # Both have to be resolved before a firmware binary may be distributed
    # again -- see docs/legal/COMPLIANCE_PLAN_zh.md, track A.
""",
        r"""    # Both *kinds* block, and neither is cured by fixing the other: the
    # copyleft pair because GPL-2.0-only and GPL-3.0 cannot be combined under
    # any single licence, the unlicensed library because compiling it in is
    # redistribution without permission.  Replacing RF24 and arduinoFFT while
    # leaving ESP32 BLE Keyboard in place would leave the firmware exactly as
    # undistributable as it is today, which is why the gate keys on `verdicts`
    # and not on the copyleft count alone.
    # See docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md for the way out.
""",
    ),
]

# --------------------------------------------------------------------------
# collect_licenses.py
# --------------------------------------------------------------------------

EDITS[COLLECT] = [
    # 1. A LicenseRef is an SPDX expression, not a filename.
    (
        r"""OUT = ROOT / "LICENSES"
""",
        r"""OUT = ROOT / "LICENSES"

# A LicenseRef is an SPDX expression, not a filename, and slugging one produces
# something no reader can parse.  A library whose licence is a LicenseRef gets
# a name here that says what the file actually is.
BESPOKE = {
    "SmartRC-CC1101-Driver-Lib": "SmartRC-CC1101-Driver-Lib-bespoke-permissive.txt",
}
""",
    ),
    # 2. Use it when naming the file.
    (
        r"""        target = f"{slug(entry['name'])}-{slug(entry['spdx'])}.txt"
""",
        r"""        target = (BESPOKE.get(entry["name"])
                  or f"{slug(entry['name'])}-{slug(entry['spdx'])}.txt")
""",
    ),
    # 3. The `unidentified` marker text described SmartRC specifically.  It no
    #    longer hits that case, so the text has to describe the class.
    (
        r"""            "This library does ship a licence file, but the text names no licence\n"
            "any tool can identify: it is a bespoke permission notice whose scope\n"
            "is never stated.  The upstream repository does declare a licence, and\n"
            "the vendored copy predates that declaration, so the intent is probably\n"
            "permissive -- but intent is not a grant, and the notice as shipped\n"
            "disclaims everything it does not expressly allow.\n"
""",
        r"""            "This library does ship a licence file, but the text names no licence\n"
            "any tool can identify and states no permission this project can rely\n"
            "on.  Nothing in it reads as a grant of rights, so the default of all\n"
            "rights reserved stands until a person reads the text and records its\n"
            "scope -- which is why the file is called out here rather than being\n"
            "treated as absent.\n"
""",
    ),
]


def apply(relative: str, pairs: list[tuple[str, str]]) -> list[str]:
    path = ROOT / relative
    if not path.is_file():
        raise SystemExit(f"FAIL {relative}: not found")
    text = path.read_text(encoding="utf-8", newline="")
    original = text
    report: list[str] = []

    for index, (old, new) in enumerate(pairs, start=1):
        label = f"{relative} edit {index}"
        if new in text:
            report.append(f"  = {label}: already applied")
            continue
        found = text.count(old)
        if found != 1:
            raise SystemExit(
                f"FAIL {label}: expected exactly 1 occurrence, found {found}.  "
                f"Anchor was:\n{old[:200]}")
        text = text.replace(old, new, 1)
        report.append(f"  + {label}: applied")

    if text != original:
        # newline="" on both sides: this file is LF in the repository, and
        # Path.write_text() would silently rewrite every line ending to CRLF on
        # Windows while `git diff --stat` showed only the edited lines.
        path.write_text(text, encoding="utf-8", newline="")
    return report


def main() -> int:
    lines: list[str] = []
    for relative, pairs in EDITS.items():
        lines.extend(apply(relative, pairs))
    print("\n".join(lines))
    print(f"\n{len(EDITS)} file(s) processed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
