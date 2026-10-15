#!/usr/bin/env python3
"""Host-side test runner for the pure-logic native tests.

The repository ships a PlatformIO `native` environment, but that environment
requires a host GCC/G++ toolchain which is not present on every developer
machine. This runner uses any host C++ compiler it can find (including the
mingw-w64 toolchain that ships inside the PlatformIO package cache, or a
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

Usage
-----
    python scripts/hostcheck/run_host_tests.py            # run everything
    python scripts/hostcheck/run_host_tests.py test_ntag_model ...
    python scripts/hostcheck/run_host_tests.py --list
    python scripts/hostcheck/run_host_tests.py --cxx <path to g++>

Exit code is the number of failing suites (0 = all green).
"""

from __future__ import annotations

import argparse
import glob
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UNITY_DIR = Path(__file__).resolve().parent / "unity"
OBJ_DIR = ROOT / "build-reports" / "hostcheck" / "obj"

# Mirrors the source filter of the `native` environment in platformio.ini.
SOURCE_GLOBS = [
    "src/core/*.cpp",
    "src/board/*.cpp",
    "src/features/**/*.cpp",
    "src/drivers/*.cpp",
    "src/platform/*.cpp",
    "src/services/SettingsService.cpp",
    "src/services/StorageService.cpp",
    "src/services/pure/*.cpp",
]

CXX_FLAGS = ["-std=gnu++1z", "-O0", "-g0", "-w", "-DHOSTCHECK_NATIVE=1"]


def find_archiver(cxx: list[str]) -> list[str] | None:
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


def _zig_prefix() -> list[str] | None:
    """Return the ``python -m ziglang c++`` prefix if a repo-local zig exists.

    ``ziglang`` is a pip wheel that bundles clang 21 plus a full libc++/libc
    sysroot, giving a working host C++ compiler on machines that have neither
    MSVC nor mingw-w64. It is installed into a repo-local venv under
    ``build-reports/hostenv``; see build-reports/README for the install line.
    """
    candidates = (
        ROOT / "build-reports" / "hostenv" / "Scripts" / "python.exe",
        ROOT / "build-reports" / "hostenv" / "bin" / "python",
    )
    for py in candidates:
        if not py.exists():
            continue
        try:
            probe = subprocess.run(
                [str(py), "-m", "ziglang", "version"],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                timeout=120,
            )
        except (OSError, subprocess.SubprocessError):
            continue
        if probe.returncode == 0:
            return [str(py), "-m", "ziglang", "c++"]
    return None


_PROBE_SRC = "int main() { return 0; }\n"
_skipped: list[str] = []


def _probe(cxx: list[str]) -> bool:
    """Compile+link+run a trivial TU; True only if the toolchain really works.

    A bare ``--version`` check is not enough: a mingw package can be present but
    unloadable (missing libgmp/libmpfr DLLs), in which case it prints a version
    and then fails every compile with an *empty* log.
    """
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "probe.cpp"
        exe = Path(tmp) / "probe.exe"
        src.write_text(_PROBE_SRC, encoding="utf-8")
        try:
            built = subprocess.run(
                [*cxx, "-std=gnu++1z", "-w", str(src), "-o", str(exe)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=300,
            )
        except (OSError, subprocess.SubprocessError):
            return False
        if built.returncode != 0:
            return False
        try:
            ran = subprocess.run(
                [str(exe)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=120,
            )
        except (OSError, subprocess.SubprocessError):
            return False
        return ran.returncode == 0


def _try(candidate: list[str], label: str) -> list[str] | None:
    if _probe(candidate):
        return candidate
    _skipped.append(label)
    return None


def find_compiler(explicit: str | None) -> list[str]:
    """Resolve the host C++ compiler to a *command prefix* (argv list).

    The result is either a bare compiler path (``["g++"]``) or a driver
    invocation (``["python", "-m", "ziglang", "c++"]``). Callers must splice
    it with ``*cxx`` instead of passing it as a single argv element.

    Resolution order: --cxx > $HOSTCXX > PlatformIO mingw32 > PlatformIO
    mingw64 > PATH g++/clang++ > repo-local ziglang venv.
    """
    if explicit:
        return shlex.split(explicit)
    env = os.environ.get("HOSTCXX")
    if env:
        return shlex.split(env)
    for pattern in (
        str(Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe"),
        str(Path.home() / ".platformio/packages/toolchain-gccmingw64/bin/g++.exe"),
    ):
        if Path(pattern).exists():
            picked = _try([pattern], pattern)
            if picked:
                return picked
    found = shutil.which("g++") or shutil.which("clang++")
    if found:
        picked = _try([found], found)
        if picked:
            return picked
    zig = _zig_prefix()
    if zig:
        picked = _try(zig, "ziglang venv")
        if picked:
            return picked
    detail = ("; skipped (probe failed): " + ", ".join(_skipped)) if _skipped else ""
    sys.exit(f"hostcheck: no working host C++ compiler found{detail}; pass --cxx <path>")


def collect_sources() -> list[Path]:
    files: list[Path] = []
    for pattern in SOURCE_GLOBS:
        files.extend(Path(p) for p in glob.glob(str(ROOT / pattern), recursive=True))
    return sorted(set(files))


def compile_command(cxx: list[str], src: Path, out: Path, extra: list[str]) -> list[str]:
    return [
        *cxx,
        *CXX_FLAGS,
        *extra,
        "-I",
        str(ROOT / "include"),
        "-I",
        str(UNITY_DIR),
        "-c",
        str(src),
        "-o",
        str(out),
    ]


def run(cmd: list[str]) -> tuple[int, str]:
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return proc.returncode, proc.stdout


def build_object(cxx: list[str], src: Path, extra: list[str]) -> tuple[Path | None, str]:
    OBJ_DIR.mkdir(parents=True, exist_ok=True)
    rel = src.relative_to(ROOT)
    out = OBJ_DIR / (str(rel).replace(os.sep, "_") + ".o")
    code, log = run(compile_command(cxx, src, out, extra))
    if code != 0:
        return None, log
    return out, ""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("suites", nargs="*", help="suite directory names to run")
    parser.add_argument("--cxx", help="path to the host C++ compiler")
    parser.add_argument("--list", action="store_true", help="list suites and exit")
    args = parser.parse_args()

    suites = sorted(p for p in (ROOT / "test" / "native").glob("*/") if p.is_dir())
    if args.suites:
        wanted = set(args.suites)
        suites = [s for s in suites if s.name in wanted]
        missing = wanted - {s.name for s in suites}
        if missing:
            sys.exit(f"hostcheck: unknown suite(s): {', '.join(sorted(missing))}")
    if args.list:
        for suite in suites:
            print(suite.name)
        return 0
    if not suites:
        sys.exit("hostcheck: no test suites found under test/native/")

    cxx = find_compiler(args.cxx)
    print(f"hostcheck: compiler = {' '.join(cxx)}")

    extra = ["-I", str(ROOT)]
    shim_obj, log = build_object(cxx, UNITY_DIR / "unity_shim.cpp", extra)
    if shim_obj is None:
        print(log)
        sys.exit("hostcheck: failed to build the unity shim")

    sources = collect_sources()
    objects: list[Path] = [shim_obj]
    build_failures: list[tuple[str, str]] = []
    for src in sources:
        obj, log = build_object(cxx, src, extra)
        if obj is None:
            build_failures.append((str(src.relative_to(ROOT)), log))
        else:
            objects.append(obj)

    if build_failures:
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

    results: list[tuple[str, int, str]] = []
    for suite in suites:
        test_file = suite / "test_main.cpp"
        if not test_file.exists():
            continue
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / (suite.name + ".exe")
            code, log = run(
                [
                    *cxx,
                    *CXX_FLAGS,
                    *extra,
                    "-I",
                    str(ROOT / "include"),
                    "-I",
                    str(UNITY_DIR),
                    str(test_file),
                    str(archive),
                    "-o",
                    str(exe),
                ]
            )
            if code != 0:
                results.append((suite.name, -1, log))
                continue
            code, log = run([str(exe)])
        results.append((suite.name, code, log))

    width = max(len(name) for name, _, _ in results) if results else 10
    failed = 0
    for name, code, log in results:
        if code == 0:
            print(f"  PASS  {name.ljust(width)}")
        else:
            failed += 1
            print(f"  FAIL  {name.ljust(width)}  (exit {code})")
            for line in log.splitlines():
                if "FAIL" in line or "error:" in line:
                    print(f"        {line.strip()}")
    print(f"hostcheck: {len(results) - failed}/{len(results)} suite(s) passed")
    return failed


if __name__ == "__main__":
    raise SystemExit(main())
