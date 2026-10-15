# build-reports

Scratch area for build/verification tooling and its output. Nothing here ships in
the firmware.

## `syntax_env_check.py` — real-toolchain syntax verification

Compiles changed translation units with the *real* environment flags by reading the
authoritative files PlatformIO itself writes:

- `nm-cyd-c5` → `.pio/c5-core/packages/framework-arduinoespressif32-libs/esp32c5/flags/*`
- `esp32-cyd` → `.pio/packages/framework-arduinoespressif32/tools/sdk/...`

```bash
python build-reports/syntax_env_check.py --env nm-cyd-c5 src/features/rfid/CardDumpFormat.cpp
python build-reports/syntax_env_check.py --env esp32-cyd --baseline   # tool-noise proof
```

`--baseline` compiles untouched files with the same flags. If they fail identically,
the failure is the tool configuration, not your change. (`esp32-cyd` always reports
`'assert' was not declared` / `'errno' was not declared` for this reason: the
pre-split framework's `tools/sdk/<chip>/include/newlib/platform_include/` shadows the
toolchain's own `assert.h` / `errno.h`.)

Not covered: linking, `extra_scripts`, `build_src_filter`, Flash/RAM footprint.

## `hostenv/` — a host C++ compiler, without a system install

`pio run` is unusable in this sandbox (the safe-delete guard's 50-file-per-round budget
is blown by the package manager before the first `Compiling` line), and the bundled
`toolchain-gccmingw32` cannot compile anything: it ships no 32-bit `libgmp-10.dll` /
`libmpfr-4.dll`, so `g++ --version` works and every real compile exits 1 with an *empty*
log.

`ziglang` is a pip wheel that bundles clang plus a complete libc++/libc sysroot, so it
gives a working host compiler with nothing installed system-wide:

```bash
python -m venv build-reports/hostenv
build-reports/hostenv/Scripts/python.exe -m pip install --upgrade pip ziglang
build-reports/hostenv/Scripts/python.exe -m ziglang version    # 0.16.0 (clang 21.1.0)
```

`hostenv/` is git-ignored. Re-create it with the three commands above on a fresh clone.

## `run_host_tests.py` — compile *and execute* the native suites

```bash
build-reports/hostenv/Scripts/python.exe scripts/hostcheck/run_host_tests.py
build-reports/hostenv/Scripts/python.exe scripts/hostcheck/run_host_tests.py --list
build-reports/hostenv/Scripts/python.exe scripts/hostcheck/run_host_tests.py test_heap_baseline
```

Compiler resolution order: `--cxx` → `$HOSTCXX` → PlatformIO mingw32 → PlatformIO
mingw64 → PATH `g++`/`clang++` → repo-local `ziglang` venv. **Every candidate is
probe-compiled first** (`int main(){return 0;}`, built, linked *and run*), because a
present-but-unloadable toolchain otherwise gets picked and then fails with an empty log.

Two things it does that are easy to get wrong:

1. Shared sources are archived into `build-reports/hostcheck/libhostcheck.a` and linked
   as an archive, mirroring the `native` PlatformIO env. Force-linking the raw `.o` files
   instead pulls in `src/core/AppKernel.o`, which needs `legacySetup` / `legacyLoop` from
   the `build_src_filter`-excluded `src/app/LegacyApp.inc`, and the link dies with
   `undefined symbol: legacySetup()`. Do **not** "fix" that with empty stubs — the stubs
   would mask genuine undefined symbols in the firmware build.
2. `-w` is passed deliberately: zig's bundled libc++ headers otherwise emit a wall of
   nullability warnings on any `<string>` / `<functional>` use.

Result on 2026-10-15: **29/29 suites, 142 test cases green.**

## `patch_*.py` — applied-and-verified source edits

Each script performs exact-string replacements with a match-count assertion, so a
silent no-op cannot happen. They are kept for provenance; re-running one whose edit is
already applied will abort harmlessly (`expected 1 match, found 0`).

| Script | What it changed |
|---|---|
| `patch_legacy_policy.py` | 21 duplicated gates in `LegacyApp.inc` → `requestOffensiveAccess()`; policy panel, wipe-SD, banner tap |
| `patch2.py`, `patch3.py`, `patch_catalog.py` | feature catalog / lifecycle edits |
| `patch_hostcheck_zig.py` | `run_host_tests.py`: compiler as a command prefix + zig detection |
| `patch_hostcheck_probe.py` | `run_host_tests.py`: probe-compile every compiler candidate |
| `patch_hostcheck_archive.py` | `run_host_tests.py`: archive shared objects before linking |
| `patch_hostcheck_fixes.py` | the nine latent defects from the first host run (see DEVELOPMENT_PLAN.md §5.5) |
| `patch_plan_hostcheck.py` | DEVELOPMENT_PLAN.md §5.4 / §5.5 / §7 / §9 |
