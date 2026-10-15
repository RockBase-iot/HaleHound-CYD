#!/usr/bin/env python3
"""Third-stage patch: link the shared sources as a static archive.

Why: ``src/core/AppKernel.cpp`` references ``legacySetup()`` / ``legacyLoop()``,
which only exist inside ``src/app/LegacyApp.inc`` -- a file the ``native``
PlatformIO env deliberately excludes (see ``build_src_filter`` in platformio.ini).
PlatformIO compiles the selected sources into a *static library*, so
``AppKernel.o`` is simply never extracted because no test suite references it.

The runner instead force-linked every ``.o``, which drags ``AppKernel.o`` in and
fails with ``undefined symbol: legacySetup()``. This patch mirrors PlatformIO by
archiving the shared objects and linking the archive, restoring the exact
"only pull what is referenced" semantics.

Also documents the ziglang fallback in the module docstring.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "scripts" / "hostcheck" / "run_host_tests.py"

OLD_DOC = '''mingw-w64 toolchain that ships inside the PlatformIO package cache) and runs
every suite under ``test/native/`` without PlatformIO, so pure-logic work can
be verified on a stock Windows install.
'''

NEW_DOC = '''mingw-w64 toolchain that ships inside the PlatformIO package cache, or a
repo-local ``ziglang`` venv under ``build-reports/hostenv``) and runs every
suite under ``test/native/`` without PlatformIO, so pure-logic work can be
verified on a stock Windows install.

Each candidate compiler is probe-compiled before use, because a mingw package
can be present yet unloadable (missing libgmp/libmpfr DLLs) and then fails every
compile with an empty log.

The shared sources are archived into ``build-reports/hostcheck/libhostcheck.a``
and linked as an archive, matching the ``native`` PlatformIO env: objects that
no suite references (e.g. ``AppKernel.o``, which needs the excluded
``src/app/LegacyApp.inc``) are never pulled in.
'''

OLD_AR = '''def _zig_prefix() -> list[str] | None:'''

NEW_AR = '''def find_archiver(cxx: list[str]) -> list[str] | None:
    """Resolve an ``ar`` matching the chosen compiler, as a command prefix."""
    if cxx[1:4] == ["-m", "ziglang", "c++"]:
        return [cxx[0], "-m", "ziglang", "ar"]
    if len(cxx) == 1:
        sibling = Path(cxx[0]).parent / "ar.exe"
        if sibling.exists():
            return [str(sibling)]
        sibling = Path(cxx[0]).parent / "ar"
        if sibling.exists():
            return [str(sibling)]
        found = shutil.which("ar") or shutil.which("llvm-ar")
        if found:
            return [found]
    return None


def _zig_prefix() -> list[str] | None:'''

OLD_ARCHIVE = '''    if build_failures:
        print(f"hostcheck: {len(build_failures)} shared source(s) failed to compile:")
        for name, log in build_failures[:3]:
            print(f"--- {name} ---")
            print(log.strip()[:2000])
        return len(build_failures)
'''

NEW_ARCHIVE = '''    if build_failures:
        print(f"hostcheck: {len(build_failures)} shared source(s) failed to compile:")
        for name, log in build_failures[:3]:
            print(f"--- {name} ---")
            print(log.strip()[:2000])
        return len(build_failures)

    ar = find_archiver(cxx)
    if ar is None:
        sys.exit("hostcheck: no archiver found to build the shared test library")
    archive = ROOT / "build-reports" / "hostcheck" / "libhostcheck.a"
    archive.parent.mkdir(parents=True, exist_ok=True)
    if archive.exists():
        archive.unlink()
    code, log = run([*ar, "rcs", str(archive), *[str(o) for o in objects]])
    if code != 0:
        print(log)
        sys.exit("hostcheck: failed to archive the shared sources")
    print(f"hostcheck: archived {len(objects)} object(s) -> {archive.name}")
'''

OLD_LINK = '''                    str(test_file),
                    *[str(o) for o in objects],
                    "-o",'''

NEW_LINK = '''                    str(test_file),
                    str(archive),
                    "-o",'''

EDITS = [
    (OLD_DOC, NEW_DOC, "document ziglang + archive semantics"),
    (OLD_AR, NEW_AR, "add find_archiver"),
    (OLD_ARCHIVE, NEW_ARCHIVE, "archive shared objects before linking"),
    (OLD_LINK, NEW_LINK, "link against the archive, not raw objects"),
]


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    for old, new, label in EDITS:
        count = text.count(old)
        if count != 1:
            print(f"ABORT: {label}: expected 1 match, found {count}")
            return 1
        text = text.replace(old, new)
        print(f"  ok  {label}")
    TARGET.write_text(text, encoding="utf-8")
    print(f"patched {TARGET}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
