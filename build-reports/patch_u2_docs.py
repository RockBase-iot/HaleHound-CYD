#!/usr/bin/env python3
"""U2 documentation: REMAINING_WORK.md, build-reports/README.md, evidence doc.

Exact-string replacements with match-count assertions, so a silent no-op cannot
happen. Re-running an already-applied patch aborts harmlessly.

Usage: python build-reports/patch_u2_docs.py
"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
applied: list[str] = []


def replace_once(path: Path, name: str, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8", newline="")
    # REMAINING_WORK.md is CRLF and the other two are LF. Patterns below are
    # written with \n, so translate them to whatever this file actually uses --
    # reading and writing with newline="" preserves the original style instead of
    # silently rewriting the whole file's line endings in the diff.
    if "\r\n" in text:
        old = old.replace("\n", "\r\n")
        new = new.replace("\n", "\r\n")
    found = text.count(old)
    if found != 1:
        sys.exit(f"{name}: expected 1 match, found {found}. Nothing was written.")
    path.write_text(text.replace(old, new), encoding="utf-8", newline="")
    applied.append(f"{name} -> {path.relative_to(ROOT)}")


# ─── REMAINING_WORK.md ─────────────────────────────────────────────────────

REMAINING = ROOT / "REMAINING_WORK.md"

replace_once(
    REMAINING,
    "R1 ROI line",
    """- **下一步最高 ROI**：① ~~新模型/驱动进 catalog~~ —— **见 §0.5：只进 catalog 不够，
  因为 runner 从来没有被启动过** → ① ~~`U1` 写 `Esp32BleCentral`~~ ✅（§0.6）→
  **真·① `U2` 接线**（授权钩子 + 渲染钩子 + 菜单读 catalog；传输层已就位但**无人 new**）→
  ② `X2` 上机跑一次真实 BLE 连接（地址反转 / live 扫描两条结论待真机复验）→
  ③ `CardDumpFormat` 落 SD（打通 B1 回归）。""",
    """- **下一步最高 ROI**：① ~~新模型/驱动进 catalog~~ —— **见 §0.5：只进 catalog 不够，
  因为 runner 从来没有被启动过** → ① ~~`U1` 写 `Esp32BleCentral`~~ ✅（§0.6）→
  ~~`U2` 接线~~ ✅（§0.7：runner 已有真实调用点，policy 门已装，渲染钩子已通）→
  **真·① `U3`**：`ChameleonUltra` 补 `FeatureId` + descriptor，让它能出现在菜单上 →
  ② `U4`：`Pn532I2cDriver` 去留决策 → ③ `U5`：16 条空壳条目补实现或摘掉 →
  ④ `X2` 上机跑一次真实 BLE 连接（地址反转 / live 扫描两条结论待真机复验）。""",
)

replace_once(
    REMAINING,
    "R2 U2 row",
    """| **U2** | **把架构接通**：`AppKernel` 用 `PolicyService` 填 `FeatureRunnerHooks`；给 `IFeature` 加渲染钩子；legacy 菜单改为读 catalog | A2+A3+A6 必须一起解：不接则「授权单点」是装饰、「菜单」是硬编码、「新屏」无处画 | **P0** |""",
    """| **U2** | ~~**把架构接通**~~ ✅ **本轮完成**（§0.7）：`AppKernel` 用 `PolicyService` 填 `FeatureRunnerHooks`（`authorize` 改为返回 `Authorization`，缺钩子时对 offensive 失败关闭）；`IFeature::draw(IFeatureCanvas&)` + `exitRequested()` + `AppKernel::setCanvas()`；蓝牙子菜单由 `FeatureMenuModel` 从 catalog 生成并经 `runner.start(id)` 派发 | —— | ✅ 完成 |""",
)

replace_once(
    REMAINING,
    "R3 section 0.7",
    "## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）",
    """## 0.7 `U2` 落地：把架构接通（2026-10-16）

**一句话**：A2/A3/A6 三条一起解了 —— runner 有了真实调用点，授权门装上了，
新屏有了可画的地方；顺带发现 U1 那次「`esp32-cyd` PASS」对 `Esp32BleCentral.cpp`
是**空的**（见下）。

### 交付物

| 文件 | 内容 |
|---|---|
| `include/core/IFeatureCanvas.h`（新） | 绘图缝。与 Arduino 无关，RGB565，`present()` 显式收帧 |
| `src/app/TftFeatureCanvas.h`（新） | `TFT_eSPI` 侧直通适配器；`drawTextClipped()` 做「最长可容纳前缀 + `~`」 |
| `include/core/IFeature.h` | `draw(IFeatureCanvas&)` 默认不画返回 false（单一覆写点，避免两个虚函数漂移）；`exitRequested()` |
| `include/core/FeatureRunner.h` / `.cpp` | `FeatureContext::canvas`；`tick()` 在 tick 之后画并 `present()`；`ownsScreen()` / `exitRequested()`；`FeatureRunnerHooks::authorize` 由 `bool` 改为 `Authorization` |
| `include/core/PolicyService.h` / `.cpp` | `globalSettingsStore()` / `globalPolicyService()`：EEPROM 后端从 `LegacyApp.inc` 搬进 core（布局不变，已配网设备状态不丢） |
| `include/core/AppKernel.h` / `.cpp` | 装真实钩子；`globalAppKernel()` 单例 |
| `include/features/FeatureMenuModel.h` / `src/features/FeatureMenuModel.cpp`（新） | 从 catalog 生成某 category 的菜单行 |
| `src/app/LegacyApp.inc` | 蓝牙子菜单改为 catalog 驱动；10 个 `case` 收敛成 `runCatalogFeature(id)`；策略动作与「诚实上报屏」各自一处 |
| `src/app/App.cpp` / `App.h` | 不再自持 kernel；装上 canvas 后交给 `globalAppKernel()` |
| `test/native/support/FakeFeatureCanvas.h`（新） | 渲染缝的宿主替身：记录而非绘制，可断言输出 |
| `test/native/test_feature_menu_model/`（新，8 用例） | 钉住「菜单 == catalog」：计数、顺序、每条可运行、标签是 catalog 自己的指针 |
| `build-reports/syntax_env_check.py` | 补 `ARDUINO`/`ARDUINO_ARCH_ESP32`/`ESP32`；目标集加 6 个文件 |

### 三个关键判断

1. **`authorize` 必须返回 `Authorization` 而不是 `bool`。**
   原钩子签名把「提示免责声明」和「Blue Team 拒绝」折叠成同一个 `Blocked`，
   而这两者需要**两个不同的屏**——调用方只能自己再问一次 policy，于是变成第二个授权点。
   现在 runner 把 `auth.detail` 当诚实上报行传出，`PolicyService` 仍是唯一权威。
   菜单侧仍然先问一次 policy，但**只为了决定画哪个屏**；runner 会再问一次决定，
   所以「忘了预检」的代价是画错屏，而不是放行。
2. **缺钩子必须失败关闭。** 原来 `AppKernel` 传 `{}`，而 `allowed()` 把空回调当放行 ⟹
   catalog 里每一条 offensive 都绕过了 `PolicyService`。现在缺 `authorize` 时
   Offensive/SensitiveData 直接 `Blocked`，Passive/Defensive 仍可跑（与
   `legacyAllowsOffensive` 同一套不对称）。
3. **渲染钩子是一个覆写点，不是一个 `drawsOwnScreen()` + `draw()` 对。**
   两个虚函数就是一个要保持同步的不变量，而漂移的后果是「声称拥有屏幕但从不绘制」。

### ⚠️ 顺带发现：U1 的 `esp32-cyd` 证据对该文件是**空的**

`esp32-cyd` 的 flag 列表里**没有 `ARDUINO`**。`Esp32BleCentral.cpp` 全文由
`#ifdef ARDUINO` 守卫，所以它在那里**整段被预处理掉**却报 PASS —— 一次「绿」的
翻译单元其实一行都没编译。补上 define 后立刻暴露一个真实缺陷：
**头文件缺 `#include <BLEDevice.h>`**（NimBLE 侧靠其它 BLE 头传递引入，Bluedroid
2.0.17 的 `BLEDevice.h` 只在自己的 `#if defined(CONFIG_BLUEDROID_ENABLED)` 里声明
`BLEDevice`，传不过来）。已修，并写进头文件注释。

**教训**：检查器报 PASS 之前，先确认那个 TU 真的被编译了。已写进 skill。

### 验证（真 toolchain，非推断）

| 目标 | 命令 | 结果 |
|---|---|---|
| `nm-cyd-c5`（C5 / 3.3.6 / NimBLE） | `syntax_env_check.py --env nm-cyd-c5` | **19/19 PASS** |
| `esp32-cyd`（ESP32 / 2.0.17 / Bluedroid） | `syntax_env_check.py --env esp32-cyd` | **19/19 PASS**（`ARDUINO` 修好后为该文件首次真正编译） |
| 基线（未改动文件）× 2 env | `--baseline` | **2/2** × 2 |
| 宿主全量 | `run_host_tests.py` | **49/49 套件 / 446 用例通过** |

### 仍未做 / 已知后果（不要读成「已完成」）

- **其余 8 个子菜单仍用手维护的数组**（WiFi / 2.4GHz / SubGHz / RFID / Jam Detect /
  SIGINT / Tools / Setting）。它们与 catalog 的漂移**依然存在**，只是蓝牙这一支不再漂移。
  机械结构已经铺好，`FeatureMenuModel` 是通用的。
- **蓝牙子菜单行数 10 → 15，竖屏行高 25 → 16 px**。用 `MenuLayout` 实测：
  `240x320` 下 15 行仍 `valid=1`、不重叠（末行 bottom 310/320）；横屏 `320x240`
  走两列、8 行、行高 23 px。**可读性是真实退化**，需要分页/滚动（照 `GattTreeView`
  的分页做法），这是一件独立的事。
- **`routeSupported` / `probePeripheral` / `preflightResources` 三个钩子仍为 null**：
  仓库里还没有路由模型、外设探测与资源经纪人接到运行时。空钩子按注释是「没有意见」，
  不是「放行」。
- **canvas 只被接线，还没有任何 feature 覆写 `draw()`**：`TftFeatureCanvas` 是真的，
  但第一个真正自己画屏的 feature（GATT 树 / 槽位表）还没写。`FakeFeatureCanvas`
  已经把宿主侧的断言路铺好了。
- **`U3` 仍然挡路**：`ChameleonUltraModel` 没有 `FeatureId`，所以它**不会**出现在蓝牙菜单上
  —— 菜单现在读 catalog，catalog 里没有它就等于没有它。

---

## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）""",
)

# ─── build-reports/README.md ───────────────────────────────────────────────

README = ROOT / "build-reports" / "README.md"

replace_once(
    README,
    "R4 README baseline claim",
    """`--baseline` compiles untouched files with the same flags. If they fail identically,
the failure is the tool configuration, not your change. (`esp32-cyd` always reports
`'assert' was not declared` / `'errno' was not declared` for this reason: the
pre-split framework's `tools/sdk/<chip>/include/newlib/platform_include/` shadows the
toolchain's own `assert.h` / `errno.h`.)""",
    """`--baseline` compiles untouched files with the same flags. If they fail identically,
the failure is the tool configuration, not your change.

No env fails its baseline any more, and that is the point: both causes were fixed
rather than documented around.

* `esp32-cyd` used to report `'assert' was not declared` / `'errno' was not declared`
  for *every* TU, touched or not. Cause: the pre-split framework's
  `tools/sdk/<chip>/include/newlib/platform_include/` shadows the toolchain's own
  `assert.h` / `errno.h`, and `soc/spinlock.h` calls `assert()` without including
  `<assert.h>` — the real build only survives because it force-includes them.
  `prepare()` now prepends the `xtensa-esp32-elf/include` directory and adds
  `-include <sysinc>/assert.h` / `errno.h` *before* the `@rsp`, which is where they
  have to go.
* `esp32-cyd` also passed no `ARDUINO`, `ARDUINO_ARCH_ESP32` or `ESP32`. That was
  worse than a bad error message. With `ARDUINO` undefined, every `#ifdef ARDUINO`
  body in that environment preprocessed to nothing, so `src/drivers/Esp32BleCentral.cpp`
  reported PASS while **not one line of it was compiled** — a green check on a
  discarded translation unit, which is worse than no check at all. Adding the define
  surfaced a real defect immediately (a missing `#include <BLEDevice.h>`), so it had
  been hiding something. Values and sources:

  | Define | Value | Where it comes from |
  |---|---|---|
  | `ARDUINO` | `20017` | `2*10000 + 0*100 + 17`; framework `package.json` is `3.20017.241212` and `esp_arduino_version.h` says MAJOR 2. Same encoding the `nm-cyd-c5` entry uses for 3.3.6 (`30306`). |
  | `ARDUINO_ARCH_ESP32` | — | `{build.arch}` in the framework's `platform.txt` recipes |
  | `ESP32` | `ESP32` | `-DESP32=ESP32`, one occurrence in the same `platform.txt`. The libdeps RF24 library picks its architecture by testing `defined(ESP32)`, not `defined(ARDUINO_ARCH_ESP32)`; without it, it takes the AVR branch and includes `<avr/pgmspace.h>`. |

  **Before trusting a PASS, check that the TU was compiled at all** — a guard that is
  false in this environment is the one failure mode `-fsyntax-only` cannot tell you
  about.""",
)

replace_once(
    README,
    "R5 README result line",
    "Result on 2026-10-15: **29/29 suites, 142 test cases green.**",
    """Result on 2026-10-16: **49/49 suites, 446 test cases green** (`test/native/` holds 50
directories; `support/` is a header-only test double and has no suite).""",
)

replace_once(
    README,
    "R6 README patch table",
    "| `patch_plan_hostcheck.py` | DEVELOPMENT_PLAN.md §5.4 / §5.5 / §7 / §9 |",
    """| `patch_plan_hostcheck.py` | DEVELOPMENT_PLAN.md §5.4 / §5.5 / §7 / §9 |
| `patch_u2_wiring.py` | U2: `LegacyApp.inc`'s ten-case Bluetooth switch -> catalog dispatch, the policy store moved to `src/core/PolicyService.cpp`, the catalog-derived submenu arrays |
| `patch_u2_docs.py` | U2 docs: `REMAINING_WORK.md` §0.7, the two stale claims in this file, the evidence doc §6 |""",
)

# ─── docs/protocol-evidence/ble-central-transport.md ───────────────────────

EVIDENCE = ROOT / "docs" / "protocol-evidence" / "ble-central-transport.md"
evidence_text = EVIDENCE.read_text(encoding="utf-8", newline="")
appendix = """
---

## 6. 校验器自身的缺口（2026-10-16 补上）—— 它掩盖了一个真实缺陷

做 `U2` 接线时发现 `esp32-cyd` 的 flag 列表里**没有 `ARDUINO`**。

后果不是「报错难看」，而是**静默**：本文件主角 `src/drivers/Esp32BleCentral.cpp`
全文由 `#if defined(ARDUINO) && (CONFIG_NIMBLE_ENABLED || CONFIG_BLUEDROID_ENABLED)`
守卫，`ARDUINO` 未定义 ⟹ 整段被预处理掉 ⟹ `-fsyntax-only` 报 **PASS**。
也就是说 §4 表格里那一行「`esp32-cyd` → `Esp32BleCentral.cpp` PASS」，
当时**一行代码都没编译**。

补上 define 之后立刻暴露的真实缺陷：

```
src/drivers/Esp32BleCentral.cpp:241:10: error: 'BLEDevice' has not been declared
```

**根因**：`include/drivers/Esp32BleCentral.h` 的 Arduino include 段里没有
`<BLEDevice.h>`。NimBLE(3.3.6) 那条路径靠其它 BLE 头**传递引入**到 `BLEDevice`，
所以 C5 编得过；Bluedroid 2.0.17 的 `libraries/BLE/src/BLEDevice.h` 把
`class BLEDevice` 放在自己的 `#if defined(CONFIG_BLUEDROID_ENABLED)` 里，
没有任何别的头会把它带进来。已修，并把这段因果写进头文件注释。

补上的宏（值都取自装好的框架文件，不是推断）：

| 宏 | 值 | 出处 |
|---|---|---|
| `ARDUINO` | `20017` | `package.json` = `3.20017.241212`，`esp_arduino_version.h` MAJOR 2 |
| `ARDUINO_ARCH_ESP32` | — | 框架 `platform.txt` 的 `{build.arch}` |
| `ESP32` | `ESP32` | `platform.txt` 的 `-DESP32=ESP32`；libdeps 的 RF24 用 `defined(ESP32)` 判断架构 |

**这条教训比缺陷本身重要**：`-fsyntax-only` 无法区分「编过」和「被预处理掉了」。
绿灯之前要先确认那个 TU 真的被编译。

## 7. 结论修订（相对 §4）

| 项 | §4 当时 | 现在 |
|---|---|---|
| `esp32-cyd` → `Esp32BleCentral.cpp` | PASS（**空**） | PASS（**真的编译了**，且修掉一个缺陷后） |
| `esp32-cyd` 基线 | 2/2 因 assert/errno 假象而失败 | 2/2 PASS |
"""
if "## 6. 校验器自身的缺口" in evidence_text:
    sys.exit("R7 evidence doc: §6 already present. Nothing was written.")
if not evidence_text.endswith("\n"):
    evidence_text += "\n"
EVIDENCE.write_text(evidence_text + appendix, encoding="utf-8", newline="")
applied.append(f"R7 evidence doc §6/§7 -> {EVIDENCE.relative_to(ROOT)}")

print("patched:")
for name in applied:
    print(f"  ok  {name}")
