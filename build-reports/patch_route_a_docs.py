#!/usr/bin/env python3
"""Record the A-route decision, the widened acceptance test, and C10.  (docs)

Three things changed in this round and each one invalidates text that is
currently in the tree:

1. **D1 is decided: A route.**  `TODO.md` and `COMPLIANCE_PLAN_zh.md` still
   present C1 as a two-way choice ("replace, or stop shipping binaries").  It
   is now one way, with a plan at `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`.

2. **The acceptance test was wrong.**  It keyed on the `strong copyleft`
   count alone, but the unlicensed ESP32 BLE Keyboard is a second, independent
   reason the binary cannot ship.  Clearing one kind leaves the firmware just
   as undistributable.  The criterion has to be the audit's exit code, which
   is already non-zero for either kind.

3. **C10 is closed, and a separate error in the same area was fixed.**  SmartRC
   is a bespoke *permissive* grant (use/edit/distribute, conditional on
   referring to the source), not an unidentified one, so its marker file is
   gone and `LICENSES/` carries the package's own text.  And rc-switch /
   TinyGPSPlus are LGPL-2.1-*or-later* -- their headers say "or (at your
   option) any later version" -- where the audit had reported the bare
   `LGPL-2.1`, which in SPDX means "-only".  The docs already said or-later,
   so the script was the thing that disagreed; both are fixed here for the
   identifiers the audit prints.

Idempotent: an edit whose replacement text is already present is reported and
skipped.

Usage:  python build-reports/patch_route_a_docs.py
"""

from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

TODO = "TODO.md"
PLAN = "docs/legal/COMPLIANCE_PLAN_zh.md"
NOTICES = "THIRD_PARTY_NOTICES.md"
REMAINING = "REMAINING_WORK.md"
README = "build-reports/README.md"

EDITS: dict[str, list[tuple[str, str]]] = {}

# --------------------------------------------------------------------------
# TODO.md
# --------------------------------------------------------------------------
EDITS[TODO] = [
    (
        r"""### `C1` 断开 copyleft 依赖（P0 · M/L）
- **目标**：让「链接出的固件二进制」在许可上成立。
""",
        r"""### `C1` 按 A 路线替换三项阻断依赖（P0 · M/L）
- **目标**：让「链接出的固件二进制」在许可上成立，判据是 `audit_licenses.py` **退出码为 0**。
  **BOSS 已定 D1 = A 路线（替换）**，方案与验收拆解见 `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`。
""",
    ),
    (
        r"""- **动作**（**只剩两条路**）：
  1. **替换** —— `RF24` → RadioLib（MIT，含 nRF24 模块）或按 Nordic 公开数据手册自写寄存器层；
     `arduinoFFT` → ESP-DSP（Apache-2.0，S3/C5 有硬件加速）或 kissfft（BSD-3）或自写 radix-2。
     上游的 `nrf24_raw.h` / `halehound_fft.h` 是**存在性证明，不是可搬运的代码**（专有许可）。
  2. **不发二进制** —— 明确仅自用/内部，仓库不产出、不提交 `.bin`。
""",
        r"""- **动作（A 路线 = 三项，不是两项）** —— 范围**已修正**：原清单只列 RF24 与 arduinoFFT，
  但 `ESP32 BLE Keyboard` 的「无授权」是**独立的**第二个阻断原因，只清 copyleft 一类，二进制依然不可分发：
  1. `arduinoFFT`（GPL-3.0）→ **自写 radix-2**（`include/hound_fft.h`）。**先做**：纯算法、宿主可验、不依赖上机。
     注意两个使用文件用的是**两套 API 风格**（`subghz_attacks.cpp` 数组随调用传入、`wifi_attacks.cpp` 构造时绑定）。
  2. `RF24`（GPL-2.0-only）→ **RadioLib**（MIT，2614★，官方含 nRF24 模块）+ 本仓适配层。
     RadioLib 的 `SPItransfer()` 是 public，四处缺口（const carrier / CRC 长度 / `isPVariant` / `flush_tx`）由我们按数据手册补。
     上游的 `nrf24_raw.h` / `halehound_fft.h` 是**存在性证明，不是可搬运的代码**（专有许可）。
  3. `ESP32 BLE Keyboard`（无授权）→ **自写 HID 层**（耦合面只有 5 个方法）。实测 **NimBLE-Arduino 不提供 HID**，
     故 `nm-cyd-c5` 侧要自己实现 HID over GATT（服务 `0x1812`）；`esp32-cyd` 侧可用 core 的 `BLEHIDDevice`。
     它与 `C3` 的 T1 改名是**同一次改动**，不要分两次做。
""",
    ),
    (
        r"""- **验收**：`python build-reports/audit_licenses.py` 的 `strong copyleft` 计数归零且**退出码为 0**；
  若走替换路线，2.4GHz 与频谱仪两条路径有宿主测试，且与替换前行为对齐（含回归）。
""",
        r"""- **验收（已修正）**：`python build-reports/audit_licenses.py` **退出码为 0**，即
  `strong copyleft`、`no grant at all`、`unclear grant` **三类同时归零**。
  ~~只看 `strong copyleft` 计数~~ —— 原判据不充分：换掉两个 GPL 而留下无授权的 BLE Keyboard，
  固件仍不可分发。逐项验收标准见方案文档 §2.3 / §3.4 / §4.4。
""",
    ),
    (
        r"""`SmartRC-CC1101-Driver-Lib` → **许可不明确**占位标记；""",
        r"""`SmartRC-CC1101-Driver-Lib` → **自定义宽松许可**（要求指明来源；一手文本副本，非占位）；""",
    ),
    (
        r"""### `C10` SmartRC 许可澄清（P1 · S）
- **为什么**：包内 `SmartRC-CC1101-Driver-Lib@2.5.7/LICENSE.txt` 是 **2021 年的自定义文本**（"You can freely use, edit or distribute it with reference to the source. For everything else, the rights of the respective owners apply!"），**早于上游仓库改标 MIT**（现行 LICENSE 为 MIT，`Copyright (c) 2026 Little Satan`）。⇒ 我们分发的是**授权范围未被写明**的那一版。
- **动作**：升级到随附 MIT 文本的版本，或向作者取书面确认；结论回填 `THIRD_PARTY_NOTICES.md`。
- **验收**：`audit_licenses.py` 中该行不再是 `UNIDENTIFIED`，且 `LICENSES/` 中不再有该库的 `LICENCE-UNRESOLVED.txt`。
""",
        r"""### `C10` SmartRC 许可澄清 ✅ 本轮已落地（P1 · S）
- **为什么**：包内 `SmartRC-CC1101-Driver-Lib@2.5.7/LICENSE.txt` 是自定义文本：**"You can freely use, edit or
  distribute it with reference to the source. For everything else, the rights of the respective owners apply!"**
  —— 这是**一项真实的授权**（可使用/修改/分发），但**附带条件**（须指明来源），且 `library.json` / `library.properties`
  **都没有 `license` 字段**，没有别的地方补充范围。
- **结论修正**：原先报成 `UNIDENTIFIED` **低估了它** —— 那会读成"授权未知"，而实际是"授权已知但非标准"。
  现按 SPDX `LicenseRef-SmartRC-bespoke-permissive` / 分类 `bespoke-permissive` 记录，
  义务以 **NOTE**（不阻断，但不许静默）打印。
- **动作（已完成）**：`LICENSES/SmartRC-CC1101-Driver-Lib-bespoke-permissive.txt` = 包内 `LICENSE.txt` **逐字节副本**
  （`cmp` 校验一致）；`NOTICE.md` / `THIRD_PARTY_NOTICES.md` 承担"指明来源"这项条件。
- **验收（已满足）**：审计该行不再是 `UNIDENTIFIED`；`LICENSES/` 中不再有该库的 `LICENCE-UNRESOLVED.txt`
  （现仅剩 `ESP32-BLE-Keyboard.LICENCE-UNRESOLVED.txt`，它对应的是**无授权**，不是同一件事，不能合并）。
- **附带修正（同轮发现）**：`rc-switch` 与 `TinyGPSPlus` 的头文件写的是 "either version 2.1 of the License,
  or (at your option) any later version"，即 **LGPL-2.1-or-later**；审计原先把它们报成裸 `LGPL-2.1`
  （SPDX 中 deprecated，含义是 `-only`），把"用户可以升级版本"说反了。已改为按措辞判版本，**且只对源文件头生效**
  （每份 GPL/LGPL 正文的 "How to Apply" 附录都含 "any later version"，套在 licence 文件上会把 RF24 的
  `GPL-2.0-only` 误升级）。两份 SPDX 2.1 文本逐字节相同，故只动标识符与文件名。
""",
    ),
    (
        r"""| `C1` | ⬜ 未开始 | `audit_licenses.py` 当前输出 `strong copyleft: 2 [arduinoFFT (GPL-3.0), RF24 (GPL-2.0)]`，exit=1 |""",
        r"""| `C1` | 🟡 A 路线已定，代码未改 | `audit_licenses.py` 当前输出 `strong copyleft: 2` + `no grant at all: 1`，exit=1；方案见 `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`（三项、分三阶段） |""",
    ),
    (
        r"""| `C10` | ⬜ 未开始 | `LICENSES/SmartRC-CC1101-Driver-Lib.LICENCE-UNRESOLVED.txt` |""",
        r"""| `C10` | ✅ 已完成 | 分类改 `bespoke-permissive`（SPDX `LicenseRef-SmartRC-bespoke-permissive`）；`LICENSES/SmartRC-CC1101-Driver-Lib-bespoke-permissive.txt` 与包内 `LICENSE.txt` `cmp` 一致；`unclear grant: 0` |""",
    ),
    (
        r"""| `C7` | 🟡 部分 | `LICENSES/` 已备 LGPL-2.1 原文；「如何重新链接」说明未写 |""",
        r"""| `C7` | 🟡 部分 | `LICENSES/` 已备 LGPL-2.1-or-later 原文；**标识符已修正为 or-later**（原报裸 `LGPL-2.1`，含义是 -only）；「如何重新链接」说明未写 |""",
    ),
    (
        r"""| **D1** | `C1` 的两条依赖：**换库**还是**自写**？ | 决定 2.4GHz、频谱仪两块怎么写；换库快但引入新依赖，自写慢但彻底干净 |""",
        r"""| **D1** | ~~两条依赖换库还是自写~~ **已定：A 路线（替换，三项）** | 2026-10-16 BOSS 决定。范围由两项修正为三项（含无授权的 BLE Keyboard）；混用策略：FFT 自写、RF24 用 RadioLib、BLE HID 自写 |""",
    ),
]

# --------------------------------------------------------------------------
# docs/legal/COMPLIANCE_PLAN_zh.md
# --------------------------------------------------------------------------
EDITS[PLAN] = [
    (
        r"""| SmartRC-CC1101-Driver-Lib | 2.5.7 | ⚠️ **包内 `LICENSE.txt` 是无法识别的自定义文本**（"You can freely use, edit or distribute it with reference to the source. For everything else, the rights of the respective owners apply!"），而**上游仓库现声明 MIT** | 不明确 | 🟡 升级到随附 MIT 文本的版本，或取得书面确认 |""",
        r"""| SmartRC-CC1101-Driver-Lib | 2.5.7 | **自定义宽松许可**（`LicenseRef-SmartRC-bespoke-permissive`）：包内 `LICENSE.txt` 允许 "freely use, edit or distribute it **with reference to the source**" —— 授权真实存在，但**附带指明来源的条件**；`library.json` / `library.properties` 均无 `license` 字段 | 宽松（附条件） | 🟢 保留：`LICENSES/` 存一手文本副本，条件由 `NOTICE.md` / `THIRD_PARTY_NOTICES.md` 满足 |""",
    ),
    (
        r"""| **A1** | 去掉 RF24（GPL-2.0-only） | ① 换 **RadioLib**（MIT，含 nRF24 模块）；② 按 Nordic 公开数据手册**自写** nRF24 寄存器级驱动（上游正是这么绕开的，但它的 `nrf24_raw.h` 是专有，**不能抄**） | M / M-L | 需要真机回归（2.4GHz 四件套） |""",
        r"""| **A1** | 去掉 RF24（GPL-2.0-only） | **已定：换 RadioLib（MIT，2614★，官方含 nRF24 模块）+ 本仓适配层**。`SPItransfer()` 为 public ⇒ const carrier / CRC 长度 / `isPVariant` / `flush_tx` 四处缺口由我们按数据手册补，不 fork 库。~~自写寄存器层~~ 保留为备选（成本高且只能上机验证） | M | 需要真机回归（`X2`） |""",
    ),
    (
        r"""| **A2** | 去掉 arduinoFFT（GPL-3.0） | ① 换 **ESP-DSP**（Apache-2.0，Espressif 官方，S3/C5 有硬件加速）② 换 kissfft（BSD-3）③ 自写 radix-2 FFT | S-M / M / S-M | 只被两台频谱仪使用 |""",
        r"""| **A2** | 去掉 arduinoFFT（GPL-3.0） | **已定：自写 radix-2**（零新依赖、保持 `double`、宿主可测）。实际 API 面只有 3 个方法 × **两套签名**（数组随调用传入 / 构造时绑定），两个使用文件各用一套。~~ESP-DSP~~ 只支持 `float32`，且会把 IDF 组件依赖铺进 `esp32-cyd`(2.0.17) 老环境 | S | 只被两台频谱仪使用；**建议先做** |""",
    ),
    (
        r"""| **A3** | 处理 ESP32 BLE Keyboard（无授权） | ① 向作者索取许可（MIT 是常态，但时间不可控）② 在已有 BLE 缝上自实现 HID-over-GATT（服务 `0x1812`，我们 U1 已经建好 `IBleCentral`）③ 以 ESP-IDF 的 NimBLE HID 示例为参照（IDF 示例为宽松许可） | S（索取）/ M（自实现） | `IBleCentral` 已就位 |""",
        r"""| **A3** | 处理 ESP32 BLE Keyboard（无授权） | **已定：自写 HID 层**。耦合面只有 5 个方法（`begin/end/isConnected/press/release/write`），单一功能（BLE Ducky）。**实测 `h2zero/NimBLE-Arduino`（Apache-2.0）不提供 HID**（19 个示例无 HID、README 无相关词）⇒ `nm-cyd-c5`(NimBLE) 侧须自写 HID over GATT（服务 `0x1812` + Report/Report Map/Protocol Mode 等标准件）；`esp32-cyd`(Bluedroid) 侧可用 core 自带 `BLEHIDDevice`（LGPL-2.1-or-later，弱 copyleft）。**与 `C3` 的 T1 改名合并做** | M–L | 无（`IBleCentral` 与 BT host 判定均已在 `REMAINING_WORK §0.6` 取证） |""",
    ),
    (
        r"""| **A6** | SmartRC 许可澄清 | 升级到随附 MIT 文本的版本（我们的 2.5.7 包内那份文本是 2021 年的旧自定义声明，早于上游改标 MIT），或向作者取书面确认 | S | 无 |""",
        r"""| **A6** | SmartRC 许可澄清 | ✅ **本轮已落地**（= `TODO.md C10`）：分类改为 `bespoke-permissive`（SPDX `LicenseRef-SmartRC-bespoke-permissive`），`LICENSES/` 存包内文本**逐字节副本**，"指明来源"这项条件由 `NOTICE.md` / `THIRD_PARTY_NOTICES.md` 承担。审计以 **NOTE**（不阻断）打印，义务不再静默 | S | 无 |""",
    ),
    (
        r"""> A5 与 A1–A3 的关系：A5 是止血，A1–A3 才是治本。但 A5 可以今天做完，而且不做完 A1–A3
> 的过程中仓库一直处于"持续再分发"状态。
""",
        r"""> A5 与 A1–A3 的关系：A5 是止血，A1–A3 才是治本。但 A5 可以今天做完，而且不做完 A1–A3
> 的过程中仓库一直处于"持续再分发"状态。

> **2026-10-16 更新：BOSS 已选定 D1 = A 路线（替换）**，A1/A2/A3/A6 的选型全部收敛（上表）。
> 执行方案、耦合面实测数据、分阶段顺序与逐项验收标准见 **`docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`**。
> 同时修正一处**判据错误**：A1–A3 是**三个并列的阻断项**，只清掉 copyleft 一类（A1+A2）而留下 A3，
> 固件依然不可分发 —— 所以验收必须键在审计**退出码**上，而不是 `strong copyleft` 计数。
""",
    ),
    (
        r"""  └── *.LICENCE-UNRESOLVED.txt             ← 无授权/许可不明的**占位标记**（让缺口在树里可见）""",
        r"""  ├── SmartRC-CC1101-Driver-Lib-bespoke-permissive.txt  ← 自定义宽松许可（附「指明来源」条件）
  └── ESP32-BLE-Keyboard.LICENCE-UNRESOLVED.txt  ← 唯一剩余标记，对应**无授权**（让缺口在树里可见）""",
    ),
    (
        r"""| **D1** | A1/A2/A3 三条依赖：**换库**还是**自写**？ | 决定 2.4GHz、频谱仪、BLE HID 三块的实现方式与工作量；换库快但引入新依赖，自写慢但彻底干净 |""",
        r"""| **D1** | ~~A1/A2/A3：换库还是自写？~~ **已定：A 路线（替换）**，且范围由两项修正为**三项**（含 A3 的无授权库） | 2026-10-16 决定。混用策略：A2 自写、A1 用 RadioLib、A3 自写；顺序 A2 → A1 → A3 |""",
    ),
    (
        r"""- **未改任何源码**：T1–T7 的改名、A1–A3 的换库都**只出方案，未动手**（D1/D2 未定）。""",
        r"""- **未改任何源码**：T1–T7 的改名、A1–A3 的换库都**只出方案，未动手**（D1 已定、D2 未定）。
- **本轮新增交付**：`docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`（A 路线方案）、A6/C10 落地（SmartRC 分类 +
  一手文本副本）、LGPL 标识符修正（`LGPL-2.1` → `LGPL-2.1-or-later`，仅对源文件头生效）。""",
    ),
]

# --------------------------------------------------------------------------
# THIRD_PARTY_NOTICES.md
# --------------------------------------------------------------------------
EDITS[NOTICES] = [
    (
        r"""| SmartRC-CC1101-Driver-Lib | 2.5.7 | unrecognised text (upstream declares MIT) | unclear | yes |""",
        r"""| SmartRC-CC1101-Driver-Lib | 2.5.7 | bespoke permissive (`LicenseRef-…`, requires source reference) | permissive, conditional | yes |""",
    ),
    (
        r"""Three findings stop this firmware from being distributable. Details, evidence and
the plan to close them: `docs/legal/COMPLIANCE_PLAN_zh.md`.""",
        r"""Three findings stop this firmware from being distributable — and the first two are
*independent* reasons, so fixing one kind leaves the firmware just as undistributable as it
is now. That is why the gate keys on this script's exit code rather than on the copyleft
count alone. Details, evidence and the plan to close them:
`docs/legal/COMPLIANCE_PLAN_zh.md`, execution plan in
`docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`.""",
    ),
    (
        r"""- **TFT_eSPI is composite.** Its own terms are MIT, but its `LICENSE.txt` also
  carries the BSD text of the Adafruit_GFX functions it imported, so the row is an
  SPDX expression rather than a single id.""",
        r"""- **TFT_eSPI is composite.** Its own terms are MIT, but its `LICENSE.txt` also
  carries the BSD text of the Adafruit_GFX functions it imported, so the row is an
  SPDX expression rather than a single id.
- **A bespoke notice can still be a grant.** SmartRC's `LICENSE.txt` says
  "You can freely use, edit or distribute it with reference to the source" —
  permissive, but conditional, and written in the author's own words instead of
  SPDX vocabulary. Reporting that as merely unidentified gets the conclusion
  wrong in the expensive direction: a reader concludes the grant is unknown when
  it is known and merely non-standard. It is recorded as a `LicenseRef` and its
  condition is printed as a NOTE — satisfied by this file and `NOTICE.md`, so it
  does not fail the gate, but it is a condition and must not be dropped when
  either file is edited.
- **`LGPL-2.1-or-later`, not bare `LGPL-2.1`.** `rc-switch` and `TinyGPSPlus`
  ship no licence file, and both headers say "either version 2.1 of the License,
  or (at your option) any later version". The bare GPL/LGPL ids in SPDX mean
  `-only`, so the earlier row had the direction of that clause backwards. The
  wording is now read for the version tag, and **only** source headers are read
  this way: every GPL/LGPL licence *text* contains "any later version" in its
  "How to Apply" appendix, so applying it to a licence file would wrongly
  upgrade RF24, whose `GPL-2.0-only` declaration has to win over the GPLv2 text
  it ships.""",
    ),
]

# --------------------------------------------------------------------------
# REMAINING_WORK.md
# --------------------------------------------------------------------------
EDITS[REMAINING] = [
    (
        r"""| **C1** | 依赖许可合规 |""",
        r"""| **C1** | 依赖许可合规（**A 路线已定**） |""",
    ),
    (
        # Single-line anchor on purpose: REMAINING_WORK.md is a CRLF file, and
        # a multi-line anchor written with plain "\n" cannot match it once
        # read_text(newline="") has preserved the "\r\n" pairs.  (TODO.md,
        # COMPLIANCE_PLAN_zh.md and THIRD_PARTY_NOTICES.md are LF, so their
        # multi-line anchors are fine.)
        r"""| BOSS | **P0** |""",
        r"""| BOSS → **A 路线已定** | **P0** |""",
    ),
    (
        r"""**替换路线需要给出替代实现与回归证据**（上游的 `nrf24_raw.h`、`halehound_fft.h` 是存在性证明，不是可搬运的代码）""",
        r"""**替换路线需要给出替代实现与回归证据**（上游的 `nrf24_raw.h`、`halehound_fft.h` 是存在性证明，不是可搬运的代码）。**2026-10-16 收敛**：BOSS 选 A 路线，且范围由两项改为**三项**（`RF24` + `arduinoFFT` + **无授权的 `ESP32 BLE Keyboard`**）—— 后者的「无授权」是独立阻断原因，只清 copyleft 一类则二进制仍不可分发，故验收须键在审计**退出码**上。方案见 `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`""",
    ),
    (
        r"""① ✅ 本轮完成（5 个「包内无 LICENSE 文件」的依赖已逐个留档：2 个补 SPDX 原文、2 个立占位标记、1 个记复合许可）""",
        r"""① ✅ 已完成（5 个「包内无 LICENSE 文件」的依赖已逐个留档：2 个补 SPDX 原文、1 个立占位标记（无授权）、1 个记自定义宽松许可、1 个记复合许可）""",
    ),
    (
        r"""| **C10** | SmartRC 许可澄清 | 包内 `LICENSE.txt` 是 **2021 年的自定义文本**（授权范围未写明），**早于上游仓库改标 MIT** | 升级到随附 MIT 文本的版本，或向作者取书面确认；结论回填 `THIRD_PARTY_NOTICES.md` | 无 | P1 |""",
        r"""| **C10** | SmartRC 许可澄清 | ✅ **已完成**：包内 `LICENSE.txt` 是自定义**宽松**授权（"freely use, edit or distribute it **with reference to the source**"），非"不明确" | 分类改 `bespoke-permissive`（SPDX `LicenseRef-SmartRC-bespoke-permissive`）+ `LICENSES/` 存一手副本 + 「指明来源」条件由 `NOTICE.md`/`THIRD_PARTY_NOTICES.md` 承担；审计以 NOTE（不阻断）打印 | 无 | ✅ P1 |""",
    ),
]


# --------------------------------------------------------------------------
# build-reports/README.md -- the provenance table has to list the three scripts
# this round added, and the audit-result figures above it changed.
# --------------------------------------------------------------------------
EDITS[README] = [
    (
        r"""Result on 2026-10-16: **2 strong copyleft that are incompatible with each other,
1 with no licence at all, 2 weak copyleft, 5 permissive** — the firmware cannot
be redistributed as built. Evidence, the trademark inventory and the plan are in
`docs/legal/COMPLIANCE_PLAN_zh.md`.""",
        r"""Result on 2026-10-16: **2 strong copyleft that are incompatible with each other,
1 with no licence at all, 2 weak copyleft, 5 permissive, 1 bespoke permissive** —
the firmware cannot be redistributed as built.

The copyleft pair and the unlicensed library are *separate* reasons, so clearing
one kind leaves the firmware just as undistributable; the gate therefore keys on
this script's exit code rather than on the copyleft count. A bespoke grant is
reported as a NOTE: it is a condition on distribution, satisfied here by
`NOTICE.md` and `THIRD_PARTY_NOTICES.md`, not a blocker. Evidence, the trademark
inventory and the plan are in `docs/legal/COMPLIANCE_PLAN_zh.md`; the execution
plan for the replacement route is `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`.""",
    ),
    (
        r"""every tracked script, not only the recent ones.""",
        r"""every tracked script, not only the recent ones.

Two styles are in use, and the difference matters when re-running one. The
one-shot patches abort harmlessly on a re-run (`expected 1 match, found 0`); the
later ones are idempotent — an edit whose replacement text is already present is
reported as applied and skipped, so a run that fails halfway can be repeated
without reverting anything first.""",
    ),
    (
        r"""| `patch_remaining_work_evidence.py` | `REMAINING_WORK.md` §0.8 (the seven facts that moved priorities), blockers X6-X9, and the new §8/§9 on compliance and the shared bus, written from the upstream field evidence |""",
        r"""| `patch_remaining_work_evidence.py` | `REMAINING_WORK.md` §0.8 (the seven facts that moved priorities), blockers X6-X9, and the new §8/§9 on compliance and the shared bus, written from the upstream field evidence |
| `patch_smartrc_licence.py` | `audit_licenses.py` + `collect_licenses.py`: SmartRC moved from `unidentified` to a `bespoke-permissive` class with an SPDX `LicenseRef`, and its obligation is printed as a non-blocking NOTE |
| `patch_lgpl_or_later.py` | `audit_licenses.py`: read the version tag out of a header notice, so `rc-switch` / `TinyGPSPlus` report `LGPL-2.1-or-later` instead of bare `LGPL-2.1` (which means `-only`). Applied to headers only -- licence texts mention "any later version" in their own appendix |
| `patch_route_a_docs.py` | the A-route decision: `TODO.md` C1/C2/C10 and the §9/§10 tables, `COMPLIANCE_PLAN_zh.md` §4/§5/§6/§8/§9, `THIRD_PARTY_NOTICES.md`, `REMAINING_WORK.md` §8, and this file's audit summary and provenance note |""",
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
        if found < 1:
            raise SystemExit(
                f"FAIL {label}: anchor not found.  Anchor was:\n{old[:260]}")
        # Anchors are deliberately narrow; a multi-hit anchor means the file has
        # drifted and the edit should be re-read rather than applied blindly.
        if found > 1:
            raise SystemExit(
                f"FAIL {label}: anchor is not unique ({found} hits).  Anchor was:\n{old[:260]}")
        text = text.replace(old, new, 1)
        report.append(f"  + {label}: applied")

    if text != original:
        # newline="" on both sides.  These files mix LF and CRLF in the tree and
        # Path.write_text() would flatten every line ending on Windows while
        # `git diff --stat` showed only the edited lines.
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
