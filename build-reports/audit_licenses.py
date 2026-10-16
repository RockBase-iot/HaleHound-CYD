#!/usr/bin/env python3
"""Audit the licences of everything this firmware links against.

Answers three questions the repository currently guesses at:

  1. What is each dependency actually licensed under?  Only one of the nine
     platformio.ini dependencies declares a licence in library.json, and none
     declares one in library.properties; for the rest the truth is in a LICENSE
     file or in the source headers, so this reads all three and records which
     one it used.
  2. Which licences are copyleft, and is any pair mutually incompatible?  A
     GPL-2.0-only library and a GPL-3.0 library cannot both be linked into one
     binary that anyone may distribute, whatever the project's own licence says.
  3. Which of them end up inside the distributed `.bin`?  Everything statically
     linked does.  The Arduino core and the ESP-IDF components are linked in by
     definition, which is why they are audited here despite never appearing in
     platformio.ini.

Two traps this script exists to avoid, both of which it fell into first:

  * Classification must use the EARLIEST match, not the first pattern in list
    order.  The canonical GPLv2 text ends by suggesting "the GNU Lesser General
    Public License", and GPLv3's section 13 names the "GNU Affero General Public
    License" -- so scanning the whole file for "LESSER" or "AFFERO" reports
    RF24 as LGPL and arduinoFFT as AGPL.  Both are plain GPL.  Matching on the
    earliest position gets the title block, which is the part that names the
    licence.
  * "No licence found" is a finding, not a gap.  A library with neither a
    declaration, nor a licence file, nor a header notice grants nothing by
    default, and such a library is reported as NO-GRANT rather than UNKNOWN.

Writes `build-reports/license-audit.json` and prints a table, so the result can
be pasted into THIRD_PARTY_NOTICES.md without transcription risk.

Usage:  python build-reports/audit_licenses.py [--json PATH]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LIBDEPS = ROOT / ".pio" / "libdeps"
PACKAGES = [
    ROOT / ".pio" / "packages",
    ROOT / ".pio" / "c5-core" / "packages",
]

# (spdx id, class, pattern).  Classification picks the earliest match in the
# text, so ordering here only breaks ties between patterns starting at the same
# offset -- which is why the more specific "LESSER"/"AFFERO" forms come first.
LICENCE_PATTERNS: list[tuple[str, str, str]] = [
    ("AGPL-3.0", "strong-copyleft", r"AFFERO\s+GENERAL\s+PUBLIC\s+LICENSE"),
    ("LGPL-2.1", "weak-copyleft", r"LESSER\s+GENERAL\s+PUBLIC\s+LICENSE"),
    ("LGPL", "weak-copyleft", r"LIBRARY\s+GENERAL\s+PUBLIC\s+LICENSE"),
    ("GPL-3.0", "strong-copyleft", r"GNU\s+GENERAL\s+PUBLIC\s+LICENSE.{0,80}?Version\s*3"),
    ("GPL-2.0", "strong-copyleft", r"GNU\s+GENERAL\s+PUBLIC\s+LICENSE.{0,80}?Version\s*2"),
    ("GPL", "strong-copyleft", r"GNU\s+GENERAL\s+PUBLIC\s+LICENSE"),
    ("Apache-2.0", "permissive", r"Apache\s+License.{0,40}Version\s*2\.0"),
    ("MIT", "permissive", r"Permission is hereby granted, free of charge"),
    ("BSD-3-Clause", "permissive", r"Neither the name of.{0,160}?may be used to endorse"),
    ("BSD-2-Clause", "permissive", r"Redistributions of source code must retain"),
    ("ISC", "permissive", r"Permission to use, copy, modify, and/or distribute"),
    ("Zlib", "permissive", r"altered source versions must be plainly marked"),
    ("Unlicense", "public-domain", r"free and unencumbered software released"),
    ("CC0-1.0", "public-domain", r"CC0\s+1\.0"),
    ("MPL-2.0", "weak-copyleft", r"Mozilla\s+Public\s+License"),
]

LICENCE_FILENAMES = (
    "LICENSE", "LICENSE.txt", "LICENSE.md", "LICENCE", "LICENCE.txt",
    "COPYING", "COPYING.txt",
)

SOURCE_SUFFIXES = (".h", ".hpp", ".c", ".cpp", ".cc", ".ino")

# Where to get a licence text when a library names one but ships no copy.  Only
# used by collect_licenses.py; recorded here so the two scripts cannot disagree.
#
# The SPDX licence-list-data texts are the canonical reference for these
# identifiers, and they are served from raw.githubusercontent.com, which is
# reachable here -- www.gnu.org timed out on the second attempt, which would
# have left the LGPL entries missing with no obvious cause.
SPDX_TEXT_URLS = {
    "LGPL-2.1": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/LGPL-2.1-only.txt",
    "LGPL-2.1-or-later": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/LGPL-2.1-or-later.txt",
    "LGPL-2.1-only": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/LGPL-2.1-only.txt",
    "LGPL-3.0": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/LGPL-3.0-only.txt",
    "GPL-2.0": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/GPL-2.0-only.txt",
    "GPL-2.0-only": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/GPL-2.0-only.txt",
    "GPL-3.0": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/GPL-3.0-only.txt",
    "AGPL-3.0": "https://raw.githubusercontent.com/spdx/license-list-data/main/text/AGPL-3.0-only.txt",
}


def classify(text: str) -> tuple[str, str]:
    """Return (spdx-ish id, class) for the licence the text names first."""
    head = text[:20000]

    # BSD-3 contains BSD-2's first clause verbatim, so earliest-match alone
    # reports a three-clause file as two-clause.  The clause that distinguishes
    # them is the one to test for, and it is unambiguous: a BSD-2 file does not
    # contain it.
    if re.search(r"Neither the name of.{0,160}?may be used to endorse", head,
                 re.IGNORECASE | re.DOTALL):
        return "BSD-3-Clause", "permissive"

    best: tuple[int, str, str] | None = None
    for spdx, kind, pattern in LICENCE_PATTERNS:
        match = re.search(pattern, head, re.IGNORECASE | re.DOTALL)
        if match is None:
            continue
        if best is None or match.start() < best[0]:
            best = (match.start(), spdx, kind)
    return (best[1], best[2]) if best else ("UNKNOWN", "unknown")


def read_text(path: pathlib.Path, limit: int | None = None) -> str:
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""
    return text[:limit] if limit else text


def licence_file(lib: pathlib.Path) -> pathlib.Path | None:
    for name in LICENCE_FILENAMES:
        path = lib / name
        if path.is_file():
            return path
    return None


def scan_licence_file(lib: pathlib.Path) -> tuple[str, str, str]:
    path = licence_file(lib)
    if path is None:
        return "", "", ""
    spdx, kind = classify(read_text(path))
    return (spdx, kind, path.name) if spdx != "UNKNOWN" else ("", "", "")


def scan_declarations(lib: pathlib.Path) -> str:
    """The `license` field of library.properties / library.json, if present."""
    props = lib / "library.properties"
    if props.is_file():
        match = re.search(r"^license\s*=\s*(.+)$", read_text(props), re.MULTILINE)
        if match:
            return match.group(1).strip()
    meta = lib / "library.json"
    if meta.is_file():
        try:
            return str(json.loads(read_text(meta)).get("license", "")).strip()
        except json.JSONDecodeError:
            return ""
    return ""


# The bare GPL/LGPL identifiers in SPDX mean "-only", because the version
# tags carry the whole meaning.  A notice that offers "any later version"
# grants something wider than that, so it has to be named differently -- an
# -or-later library may be relicensed under the next version, which an -only
# one may not.
OR_LATER = {
    "LGPL-2.1": "LGPL-2.1-or-later",
    "LGPL-3.0": "LGPL-3.0-or-later",
    "GPL-2.0": "GPL-2.0-or-later",
    "GPL-3.0": "GPL-3.0-or-later",
    "AGPL-3.0": "AGPL-3.0-or-later",
}


def or_later(text: str) -> bool:
    """True when the notice offers the recipient the next version too."""
    return re.search(
        r"either\s+version\s+[0-9.]+\s+of\s+the\s+License,\s*"
        r"or\s*\(?at\s+your\s+option\)?\s*any\s+later\s+version",
        text, re.IGNORECASE | re.DOTALL) is not None


def scan_headers(lib: pathlib.Path, max_files: int = 60) -> tuple[str, str, str]:
    """Some libraries ship no licence file but do carry a header notice."""
    checked = 0
    for path in sorted(lib.rglob("*")):
        if path.suffix.lower() not in SOURCE_SUFFIXES or not path.is_file():
            continue
        checked += 1
        if checked > max_files:
            break
        body = read_text(path, limit=4096)
        spdx, kind = classify(body)
        if spdx != "UNKNOWN":
            if or_later(body):
                spdx = OR_LATER.get(spdx, spdx)
            return spdx, kind, path.relative_to(lib).as_posix()
    return "", "", ""


def library_version(lib: pathlib.Path) -> str:
    for name in ("library.properties", "library.json"):
        path = lib / name
        if not path.is_file():
            continue
        text = read_text(path)
        match = re.search(r"^version\s*=\s*(.+)$", text, re.MULTILINE)
        if match:
            return match.group(1).strip()
        try:
            return str(json.loads(text).get("version", "")).strip()
        except json.JSONDecodeError:
            return ""
    return ""


def audit_libraries() -> list[dict]:
    seen: dict[str, dict] = {}
    for env_dir in sorted(LIBDEPS.glob("*")):
        if not env_dir.is_dir():
            continue
        for lib in sorted(env_dir.iterdir()):
            if not lib.is_dir():
                continue
            entry = seen.get(lib.name)
            if entry is None:
                entry = seen[lib.name] = {
                    "name": lib.name,
                    "version": library_version(lib),
                    "declared": scan_declarations(lib),
                    "envs": [],
                }
            if env_dir.name not in entry["envs"]:
                entry["envs"].append(env_dir.name)
            if entry.get("spdx"):
                continue
            spdx, kind, source = scan_licence_file(lib)
            if not spdx:
                shipped = licence_file(lib)
                if shipped is not None:
                    # A licence file is present but names no licence we know.
                    # Reporting this as "no grant" would be as wrong as
                    # reporting it as permissive: SmartRC ships a bespoke
                    # permission notice whose scope nobody can state, and the
                    # whole point of the audit is not to guess which way.
                    entry.update(spdx="UNIDENTIFIED", kind="unidentified",
                                 evidence=f"{shipped.name} (unrecognised text)")
                    continue
                spdx, kind, source = scan_headers(lib)
            if spdx:
                entry.update(spdx=spdx, kind=kind, evidence=source)
            else:
                entry.update(spdx="", kind="", evidence="")

    for entry in seen.values():
        # A declaration is the maintainer's own statement of intent and is kept
        # even when it is narrower than the shipped text: RF24 declares
        # "GPL-2.0-only" while shipping the plain GPLv2 text, and the "-only"
        # matters, because it is what makes the library incompatible with
        # anything GPL-3.0.
        declared = (entry["declared"] or "").upper()
        if not entry["spdx"]:
            if declared:
                entry["spdx"] = declared
                entry["kind"] = "strong-copyleft" if "GPL" in declared else "unknown"
                entry["evidence"] = "library.json/properties declaration"
            else:
                entry["spdx"] = "NO LICENCE FOUND"
                entry["kind"] = "no-grant"
                entry["evidence"] = "no declaration, no licence file, no header notice"
    # Composite licences: a library whose LICENSE file embeds borrowed sources
    # under a second licence cannot be described by whichever text matched
    # first.  TFT_eSPI's own terms are MIT, but its LICENSE.txt also carries the
    # BSD text of the Adafruit_GFX functions it imported, so a row reading just
    # "BSD-3-Clause" would misdescribe the library and a row reading just "MIT"
    # would lose the obligation it inherited.  An SPDX expression keeps both.
    COMPOSITE = {
        "TFT_eSPI": ("MIT AND BSD-3-Clause", "permissive",
                     "LICENSE.txt (composite: own terms plus embedded Adafruit_GFX)"),
    }
    for name, (spdx, kind, evidence) in COMPOSITE.items():
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

    return sorted(seen.values(), key=lambda e: e["name"].lower())


def audit_frameworks() -> list[dict]:
    """The toolchain packages: the Arduino core is statically linked into every
    build, so it belongs in the same audit even though platformio.ini never
    mentions it.  Its licence lives in package.json, not in a LICENSE file."""
    results: list[dict] = []
    seen_paths: set[str] = set()
    labels = {
        "framework-arduinoespressif32-libs":
            "ESP-IDF prebuilt libs (framework-arduinoespressif32-libs)",
        "framework-arduinoespressif32":
            "ESP32 Arduino core (framework-arduinoespressif32)",
    }
    for base in PACKAGES:
        if not base.is_dir():
            continue
        for pkg in sorted(base.iterdir()):
            key = next((k for k in labels if pkg.name.startswith(k)), None)
            if key is None:
                continue
            real = pkg.resolve().as_posix()
            if real in seen_paths:  # @src-* mirrors of an already-seen package
                continue
            seen_paths.add(real)

            version, declared = "", ""
            meta = pkg / "package.json"
            if meta.is_file():
                try:
                    data = json.loads(read_text(meta))
                    version = str(data.get("version", ""))
                    declared = str(data.get("license", ""))
                except json.JSONDecodeError:
                    pass

            spdx, kind, source = scan_licence_file(pkg)
            if not spdx and declared:
                spdx, kind = declared, ("weak-copyleft" if "LGPL" in declared.upper()
                                        else "unknown")
                source = "package.json declaration"
            results.append({
                "name": labels[key],
                "path": pkg.relative_to(ROOT).as_posix(),
                "version": version,
                "spdx": spdx or "UNKNOWN",
                "kind": kind or "unknown",
                "declared": declared,
                "evidence": source,
            })
    return results


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", default="build-reports/license-audit.json")
    args = parser.parse_args()

    libs = audit_libraries()
    frameworks = audit_frameworks()
    report = {"libraries": libs, "frameworks": frameworks}

    out = ROOT / args.json
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                   encoding="utf-8", newline="")

    print(f"{'library':33s} {'version':9s} {'licence':17s} {'class':17s} evidence")
    print("-" * 104)
    for e in libs:
        print(f"{e['name'][:32]:33s} {e['version'][:8]:9s} {e['spdx'][:16]:17s} "
              f"{e['kind']:17s} {e['evidence']}")
    print()
    for e in frameworks:
        print(f"{e['name'][:51]:52s} {e['version'][:14]:15s} {e['spdx'][:16]:17s} "
              f"{e['kind']:17s} {e['evidence']}")

    blockers = [e for e in libs if e["kind"] == "strong-copyleft"]
    weak = [e for e in libs if e["kind"] == "weak-copyleft"]
    nogrant = [e for e in libs if e["kind"] == "no-grant"]
    unclear = [e for e in libs if e["kind"] == "unidentified"]
    bespoke = [e for e in libs if e["kind"] == "bespoke-permissive"]
    okay = [e for e in libs if e["kind"] == "permissive"]
    print()
    print(f"strong copyleft : {len(blockers):2d}  "
          f"{[e['name'] + ' (' + e['spdx'] + ')' for e in blockers]}")
    print(f"weak copyleft   : {len(weak):2d}  {[e['name'] for e in weak]}")
    print(f"no grant at all : {len(nogrant):2d}  {[e['name'] for e in nogrant]}")
    print(f"unclear grant   : {len(unclear):2d}  "
          f"{[e['name'] + ' (' + e['evidence'] + ')' for e in unclear]}")
    print(f"permissive      : {len(okay):2d}  {[e['name'] for e in okay]}")
    print(f"bespoke grant   : {len(bespoke):2d}  "
          f"{[e['name'] + ' (' + e['evidence'] + ')' for e in bespoke]}")

    verdicts = []
    gpl2 = any(e["spdx"].startswith("GPL-2") for e in blockers)
    gpl3 = any(e["spdx"].startswith("GPL-3") for e in blockers)
    if gpl2 and gpl3:
        verdicts.append(
            "GPL-2.0-only and GPL-3.0 are both linked in.  They are mutually "
            "incompatible, so no single licence can cover the combined binary: "
            "it is not distributable as built.")
    if nogrant:
        verdicts.append(
            f"{len(nogrant)} libraries grant no licence at all "
            f"({', '.join(e['name'] for e in nogrant)}).  Compiled into the "
            f"firmware this is redistribution without permission.")
    if weak:
        verdicts.append(
            f"{len(weak)} libraries are weak copyleft "
            f"({', '.join(e['name'] for e in weak)}).  Static linking is "
            f"permitted, but the obligation to allow relinking has to be met "
            f"by shipping the library sources and the objects needed to relink.")
    print()
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
    print(f"\nwrote {out.relative_to(ROOT)}")

    # Non-zero when anything blocks distribution, so this can be the CI gate
    # rather than a report somebody has to remember to read.  The repository is
    # currently non-compliant, so this fails today on purpose:
    #   strong copyleft  RF24 (GPL-2.0-only) + arduinoFFT (GPL-3.0)
    #   no grant         ESP32 BLE Keyboard
    # Both *kinds* block, and neither is cured by fixing the other: the
    # copyleft pair because GPL-2.0-only and GPL-3.0 cannot be combined under
    # any single licence, the unlicensed library because compiling it in is
    # redistribution without permission.  Replacing RF24 and arduinoFFT while
    # leaving ESP32 BLE Keyboard in place would leave the firmware exactly as
    # undistributable as it is today, which is why the gate keys on `verdicts`
    # and not on the copyleft count alone.
    # See docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md for the way out.
    if verdicts:
        print("\nFAIL: the dependency set as built cannot be distributed.")
        return 1
    print("OK: every dependency is permissively licensed or weak-copyleft "
          "with its obligations met.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
