# 授权与商标合规方案（HaleHound-CYD fork / NM-CYD-C5）

> 状态：2026-10-16 起草，事实全部来自本仓库与上游一手材料（commit、LICENSE 原文、GitHub API、
> 已装依赖的许可文件），取证命令与结果见 `docs/upstream-analysis/USER_EVIDENCE_zh.md` 与
> `build-reports/license-audit.json`。
>
> **这不是法律意见。** 本文做的是"工程侧合规工程"：把事实钉死、把可执行的处置写清楚、
> 把不可自动化的部分（专利、商标注册、换证追溯性）标出来交给律师/代理人。
> 文末 §8 列出**只有 BOSS 能定**的决策项。

---

## 0. 结论摘要（先看这段）

你问的三件事，答案分别是：

| 问题 | 结论 |
|---|---|
| **授权问题怎么规避** | 上游换证**不追溯**我们手上这份副本——我们拿到的就是 MIT。但 MIT 只解决"版权许可"，它在**商标、专利、归属声明**三处留了缺口，而且我们**当前产出的固件二进制在许可上根本不可分发**（见 §4）。所以"规避"= ① 停止把不可分发的二进制放进仓库；② 补上归属与第三方声明；③ 断掉 copyleft 依赖。 |
| **HaleHound 商标怎么办** | MIT **不授予商标权**（它对此完全沉默，沉默≠授权）。可以做的是"指示性使用"——在归属说明里写"基于 HaleHound-CYD"。不可以做的是把它当产品名/广播名/包装名。当前有 **4 处用户可见或对外广播的商标使用**必须改（§3），其中 `BleKeyboard("HaleHound KB", "HaleHound")` 最严重——那是对着附近所有手机广播。 |
| **依赖许可合规** | 审计结果：**2 个强 copyleft 互不相容（RF24 GPL-2.0-only + arduinoFFT GPL-3.0）**、**1 个完全无授权（ESP32 BLE Keyboard）**、**2 个弱 copyleft 需履行 relink 义务**、3 个许可文本不明确/不标准。⇒ **当前依赖集产出的 `.bin` 任何人都不可以分发**，我们自己的 MIT 也无济于事。 |

三件事的先后关系是硬的：**必须先断 copyleft 依赖，才谈得上给用户发许可**——因为在
GPL-2.0-only ⊕ GPL-3.0 的组合下，**没有**任何一份许可能覆盖整个二进制，我们无权对客户授权。

---

## 1. 我们到底持有什么：许可链与证据

这一节是整套结论的地基。上一轮我们以为"上游是纯分发仓库、只能按 release notes 独立实现"——
**这个判断是错的**，本轮查清了真实血缘。

### 1.1 血缘（实测）

| 事实 | 证据 |
|---|---|
| 本仓库**带着上游的源码历史**，不是重写 | 根提交 `e0b7cba`（2026-02-17，"HaleHound-CYD v2.5.0 — Initial release"），作者 `JMFH` = 上游作者 |
| 116 个提交中，93 个是上游作者的（2026-02-17 → 2026-03-16），22 个是我们的（2026-07-27 起） | `git log --format='%an' \| uniq -c` |
| **分叉点 = `68bbfe6`（2026-03-16，v3.4.0 之后）**，之后上游的提交（v3.5.0 → v4.0）我们**没有** | 我们的历史止于此；上游 tag v3.5.0 起的内容不在库里 |
| 上游**改写过历史**，两边已无共同祖先 | `git merge-base HEAD FETCH_HEAD` 为空；上游 12 个 tag（v2.6.0→v3.4.0）全部指向同一个 commit `3b8633f3` |
| **上游换证时间点：v3.5.0 仍是 MIT，v3.6.2 已是专有** | 逐 tag 抓 `LICENSE` 全文比对：v3.5.0 = 14991 字节含 "MIT LICENSE"；v3.6.2 = 16575 字节含 "PROPRIETARY LICENSE" |
| 我们的分叉点**在换证之前** | 2026-03-16 < 换证（v3.5.0/v3.6.2 之间） |

### 1.2 我们手上的授权

`LICENSE`（仓库根，324 行）是**上游 v3.4.0 时代的 MIT 文本**，双著作权：

- `Copyright (c) 2026 Jesse C. Hale (HaleHound-CYD Edition)`
- `Copyright (c) 2023 CiferTech (original ESP32-DIV)`

**MIT 的授权对已获得的副本不可撤销**：上游可以停止用 MIT 分发后续版本，但无法收回对我们
手上这份副本已经作出的授权。这一点在开源法务上是通行理解。

> ⚠️ 追认项：这个"不可撤销"的强度是**唯一值得让律师确认一句**的地方（争议点在于
> "授权可否在受让人依赖之前撤回"）。工程上我们按"已授予且不可撤回"处理，因为
> ① 我们已经基于它投入了开发；② 上游在换证前从未声明过溯及力。

### 1.3 但 MIT 仍然"影响后续开发、分发、授权"——你的直觉是对的

这正是 MIT 看似宽松却仍约束我们的四处：

1. **归属义务是永久的。** MIT 要求"版权声明与许可声明必须包含在本软件的所有副本或实质性
   部分中"。⇒ 产品化时**不能把上游痕迹抹掉当作自己的原创**，固件、文档、发布页都得能指回上游。
   这一条与"去商标"并不冲突，但两者必须分开做：**去商标 ≠ 去归属**。
2. **MIT 不含专利授权。** 相比 Apache-2.0 的显式专利授予，MIT 对专利只字未提。射频繁涉及
   调制/跳频等专利密集领域，**上游若持有相关专利，MIT 不构成专利许可**。这条我们无法自查，
   只能列为外部意见项（§8）。
3. **商标不随许可走。** 详见 §3。
4. **换证这个动作本身就是信号。** 作者把自己作品的许可从 MIT 收紧到专有、并禁止再分发与
   反向工程，说明他对分发控制有明确意图。不追溯不等于没风险：任何"从上游再拿一点东西"的
   动作（截图、文案、美术、甚至 release notes 的**表达**）都必须先核对**当时的**许可。
   事实性信息（如某个 issue 里用户报告了什么 bug）可以用，**表达**不能抄。

---

## 2. 与上游的边界：能做什么、不能做什么

| 动作 | 允许？ | 依据 |
|---|---|---|
| 继续开发、修改我们手上的这份代码 | ✅ | MIT 已授予修改与分发权 |
| 商业分发、闭源、以自有品牌出货 | ✅（前提：§3 改名 + §4 断 GPL） | MIT 允许 sublicense 与再许可 |
| 在本文档/README 写"基于 HaleHound-CYD" | ✅ | 归属义务，也是指示性使用 |
| 复制上游 v3.6.2+ 的源码/文档/美术/文案 | ❌ | 那是专有文本（禁止衍生、再分发、改署名） |
| 使用 `flash.halehound.com`、自己的 `.bin` 走它的刷写器 | ❌ | 绕过其分发控制，且涉商标 |
| 把 `HaleHound` 用作自己产品的名字/广播名/包装 | ❌ | 商标（§3） |
| 声称作者是上游、或暗示上游背书 | ❌ | 专有许可明文禁止；商标法上属虚假关联 |
| 拿上游的 ASCII logo / splash 美术做我们的产品素材 | ❌（即使 MIT 期覆盖美术） | MIT 覆盖版权、**不覆盖商标**；该美术本身即商标载体 |
| 从 `git log` 里读上游每个版本的变更**事实** | ✅ | 事实不受版权保护 |

实践规则：**"能读，不能抄；能指，不能用。"**

---

## 3. 商标：风险分级与实际使用清单

### 3.1 法理要点

- MIT 通篇未提商标 ⇒ **没有商标许可**，也不存在默示许可（Apache-2.0 §6 会把这条显式写出来，
  MIT 连写都不写，效果一样：不授予）。
- 上游在用这个牌子做**同一类商品**（固件 + 该固件的硬件，官网/商店在售）⇒ 一旦我们把它
  用在自产产品上，**混淆可能性极高**，这是商标侵权判定的核心要件。
- 我们**不需要**去查上游是否注册：未注册商标通过使用也产生权利（普通法/在先使用），
  而且"注册与否"只影响救济力度，不影响"我们该不该用"。
- **允许的形态只有指示性使用**：说明来源、且不暗示关联或背书。

### 3.2 已核实的商标使用点（按风险从高到低）

| # | 位置 | 形态 | 风险 | 处置 |
|---|---|---|---|---|
| **T1** | `src/legacy/bluetooth_attacks.cpp:9153`（根目录副本同在）<br>`new BleKeyboard("HaleHound KB", "HaleHound", 100)` | **对外广播的设备名与厂商名** | **最高**——设备对附近所有手机自称 HaleHound，这是把商标用作商品标识 | 改为自有产品名（NM 系列） |
| **T2** | `src/app/LegacyApp.inc:2782 / 4186 / 4199`<br>`drawGlitchTitle(..., "HALEHOUND")` | 开机/标题画面**屏幕文字** | 高——买家第一眼看到的品牌 | 改为自有产品名 |
| **T3** | `include/icon.h:1460-1464`<br>`bitmap_halehound_splash[] PROGMEM` | 上游**美术资产**编进固件 | 高——商标图形的载体 | 换自有启动图；原图仅留在取证档案 |
| **T4** | `src/app/LegacyApp.inc:2527 / 2579`<br>`tft.print("By: HaleHound (JMFH)")` | About 画面署名 | 中——可保留但**必须改写成归属声明形态** | 改为 "Based on HaleHound-CYD by Jesse C. Hale (MIT)" |
| **T5** | `flash_package/` 与 `dist/` 共 **11 个已跟踪 `.bin`**（18.8 MB），文件名 `HaleHound-*.bin` | 分发物**文件名** | 中高——且见 §4，这些文件本身就不该在仓库里 | 从仓库移除（§4 A5） |
| **T6** | `README.md`（23 处）、`LICENSE`（16 处，整份是上游品牌文档）、仓库名 `HaleHound-CYD` | 项目层品牌 | 中 | README/LICENSE 重写（§6）；仓库名属 fork 描述性命名，可暂留但要与产品名切开 |
| **T7** | 85 个文件共 **4359 处**，绝大多数是颜色宏 `HALEHOUND_MAGENTA` / `HOTPINK` / `GUNMETAL` / `DARK` | **内部标识符** | 低（不可见、不对外） | 机械重命名，随 §4 的改动一起做 |

### 3.3 命名与声明模板

- 产品名：使用自有品牌（NM 系列 / WORKBUDDY），**不含 HaleHound**。
- 必带的归属行（固件 About 页、README、发布页、包装）： 

  > Based on HaleHound-CYD by Jesse C. Hale (MIT). HaleHound-CYD itself derives from
  > ESP32-DIV by CiferTech (MIT).

- 必带的不关联声明：

  > Not affiliated with, endorsed by, or sponsored by the HaleHound project,
  > Jesse C. Hale, or CiferTech.

- 商标致谢（非许可，仅声明权利归属）：

  > "HaleHound" is a mark of its respective owner and is used here only to identify
  > the origin of this work.

---

## 4. 依赖许可：审计结果与三个阻断项

审计由 `build-reports/audit_licenses.py` 产出（可复跑，结果落 `build-reports/license-audit.json`）。
**脚本会因阻断项返回非零**，可以直接当 CI 门禁用。

```
python build-reports/audit_licenses.py      # 打印表格；有阻断项则 exit 1
```

### 4.1 全表（逐条核对过原文，非推断）

| 依赖 | 版本 | 许可 | 类别 | 处置 |
|---|---|---|---|---|
| **RF24** | 1.4.11 | **GPL-2.0-only**（`library.json` 声明；LICENSE 为 GPLv2 全文） | 强 copyleft | 🔴 **替换或自写** |
| **arduinoFFT** | 2.0.4 | **GPL-3.0**（LICENSE 全文 "Version 3, 29 June 2007"） | 强 copyleft | 🔴 **替换或自写** |
| **ESP32 BLE Keyboard** | 0.3.2 | **无任何授权**（无声明、无 LICENSE 文件、无文件头声明；GitHub 同样报告无许可） | 无授权 | 🔴 **替换或取得授权** |
| ESP32 Arduino core | 3.20017.241212 / 3.3.6 | LGPL-2.1-or-later（`package.json`） | 弱 copyleft | 🟡 保留 + 履行 relink 义务 |
| rc-switch | 2.6.4 | LGPL-2.1-or-later（`RCSwitch.h` 头声明，文件按 "or later"） | 弱 copyleft | 🟡 同上，或自写（很小） |
| TinyGPSPlus | 1.1.0 | LGPL-2.1-or-later（`TinyGPS++.h` 头声明） | 弱 copyleft | 🟡 同上，或自写 NMEA 解析 |
| SmartRC-CC1101-Driver-Lib | 2.5.7 | ⚠️ **包内 `LICENSE.txt` 是无法识别的自定义文本**（"You can freely use, edit or distribute it with reference to the source. For everything else, the rights of the respective owners apply!"），而**上游仓库现声明 MIT** | 不明确 | 🟡 升级到随附 MIT 文本的版本，或取得书面确认 |
| TFT_eSPI | 2.5.43 | MIT **AND** BSD-3-Clause（复合：自身条款 + 内嵌 Adafruit_GFX 部分） | 宽松 | 🟢 保留，随附声明 |
| Adafruit PN532 | 1.3.4 | BSD-3-Clause（三条款齐全） | 宽松 | 🟢 保留 |
| Adafruit BusIO | 1.17.4 | MIT | 宽松 | 🟢 保留 |
| ArduinoJson | 7.4.3 | MIT | 宽松 | 🟢 保留 |
| Unity | 2.6.1 | MIT | 宽松 | 🟢 仅测试用，不进固件 |

> 审计脚本自己踩过两个坑，都已修正并写进脚本注释，因为同一类误判很容易再次发生：
> ① 按"模式列表顺序"分类会把 **GPLv2 判成 LGPL**（GPLv2 正文末尾建议改用 "GNU **Lesser**
> General Public License"）、把 **GPLv3 判成 AGPL**（GPLv3 §13 提到 "GNU **Affero** General
> Public License"）——改成"按最早出现位置匹配标题行"；
> ② **BSD-3 全文包含 BSD-2 的第一条款**，纯"最早匹配"会把三条款判成两条款——改成先判
> BSD-3 的区分性条款。

### 4.2 阻断项一：GPL-2.0-only 与 GPL-3.0 互不相容

- `RF24` 声明的是 **`GPL-2.0-only`**——"only"意味着**就是 v2，不含 "or later"**。
- `arduinoFFT` 是 **GPL v3**。
- FSF 的不相容清单里，**GPLv2-only 与 GPLv3 不能合并进同一个作品**。
- ⇒ 链接出的固件**没有任何一份许可能覆盖整体**：既不能按 GPLv2 发（因为含 v3 部分），
  也不能按 GPLv3 发（因为 v2-only 部分不允许升版），**更不可能按我们的 MIT 发**。
- 我们自己的 MIT 只管我们写的部分，救不了这个组合。

**这不是"倾向 GPL 所以要注意"，而是"整包不可分发"。**

### 4.3 阻断项二：ESP32 BLE Keyboard 完全无授权

- 无许可 = 默认**保留所有权利**。
- 它被 `#include <BleKeyboard.h>` 实际引用并 `new BleKeyboard(...)`，**编译进固件**。
- 编译进二进制并分发 = 对该库的再分发。⇒ 无授权下的再分发。

### 4.4 阻断项三：11 个已跟踪的 `.bin` 就是"分发行为"本身

`flash_package/*.bin`（10 个）与 `dist/*.bin` 已入 git（`dist/` 虽在 `.gitignore` 里，
但文件是**在加 ignore 之前**提交的，仍被跟踪）。**仓库公开 = 分发**。
这些二进制同时携带 §4.2 的不可分发组合、§4.3 的无授权库，以及 §3 的商标文件名。

---

## 5. 处置方案：决策矩阵

| 编号 | 动作 | 方案 | 规模 | 依赖 |
|---|---|---|---|---|
| **A1** | 去掉 RF24（GPL-2.0-only） | ① 换 **RadioLib**（MIT，含 nRF24 模块）；② 按 Nordic 公开数据手册**自写** nRF24 寄存器级驱动（上游正是这么绕开的，但它的 `nrf24_raw.h` 是专有，**不能抄**） | M / M-L | 需要真机回归（2.4GHz 四件套） |
| **A2** | 去掉 arduinoFFT（GPL-3.0） | ① 换 **ESP-DSP**（Apache-2.0，Espressif 官方，S3/C5 有硬件加速）② 换 kissfft（BSD-3）③ 自写 radix-2 FFT | S-M / M / S-M | 只被两台频谱仪使用 |
| **A3** | 处理 ESP32 BLE Keyboard（无授权） | ① 向作者索取许可（MIT 是常态，但时间不可控）② 在已有 BLE 缝上自实现 HID-over-GATT（服务 `0x1812`，我们 U1 已经建好 `IBleCentral`）③ 以 ESP-IDF 的 NimBLE HID 示例为参照（IDF 示例为宽松许可） | S（索取）/ M（自实现） | `IBleCentral` 已就位 |
| **A4** | 履行 LGPL-2.1 的静态链接义务 | Arduino core 无法替换，必须履约：随附库源码 + 我们的修改 + **可重新链接的手段**（对象文件或等价物）。仓库本就源码可见 ⇒ 补一份"如何重新链接"的构建说明，并声明按请求提供 `.o` | S | 无 |
| **A5** | **立刻做、成本最低、收益最大** | 把 `flash_package/*.bin` 从 git 移除并纳入 ignore（`dist/` 已在 ignore，补 `flash_package/*.bin`）。**这一步直接把"我们在分发一个不可分发的二进制"这个事实从主线上去掉** | S | 无 |
| **A6** | SmartRC 许可澄清 | 升级到随附 MIT 文本的版本（我们的 2.5.7 包内那份文本是 2021 年的旧自定义声明，早于上游改标 MIT），或向作者取书面确认 | S | 无 |

> A5 与 A1–A3 的关系：A5 是止血，A1–A3 才是治本。但 A5 可以今天做完，而且不做完 A1–A3
> 的过程中仓库一直处于"持续再分发"状态。

---

## 6. 衍生声明体系（本轮已落地）

### 6.1 文件结构（现状）

```
LICENSE                                    ← 项目自身许可（重写：我方 MIT + 派生链前言 + 商标声明）
NOTICE.md                                  ← 派生、归属、商标与不关联声明（新）
THIRD_PARTY_NOTICES.md                     ← 全部依赖的许可与处置（新）
LICENSES/                                  ← 全部许可原文，逐字副本（新，14 份）
  ├── HaleHound-CYD-MIT.txt                ← 上游 MIT 原文，逐字保留（换证前的证据）
  ├── RF24-GPL-2.0.txt / arduinoFFT-GPL-3.0.txt / ...  ← 依赖原文
  ├── framework-arduinoespressif32-LGPL-2.1-or-later.txt
  └── *.LICENCE-UNRESOLVED.txt             ← 无授权/许可不明的**占位标记**（让缺口在树里可见）
build-reports/audit_licenses.py            ← 审计（可当 CI 门禁，有阻断项 exit 1）
build-reports/collect_licenses.py          ← 按审计结果生成 LICENSES/，不誊抄
```

**一个关键设计**：`LICENSE` 里那 324 行上游品牌文档**不是被删掉**，而是逐字移到
`LICENSES/HaleHound-CYD-MIT.txt`。因为 MIT 的归属义务要求保留版权与许可声明——
**改名前必须先把归属搬好，否则"去品牌"就变成了"去归属"。**

### 6.2 逐文件 SPDX 头（政策已定，落地待做）

新写的文件、以及我们从上游改过的文件，都应有：

```cpp
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 RockBase-iot
// SPDX-FileCopyrightText: 2026 Jesse C. Hale (HaleHound-CYD)
// SPDX-FileCopyrightText: 2023 CiferTech (original ESP32-DIV)
```

- 纯自写文件：只写我方行。
- 改自上游的文件：**上游行必须保留**，我方行另加。
- 第三方库原样引用的文件：不动，其自身头声明为准。

> 这一项不做成一次性全仓改动，因为它会产生千行级 diff、把许可改动淹没。列为 `TODO.md` 的
> 独立任务，配套脚本化应用。

### 6.3 为什么"增加必要时声明"不是加一段话就够

MIT 的归属义务是**分发条件**，不是礼貌：不保留声明，就等于超出了 MIT 授予的范围。
同理，BSD-3 要求保留三条款（含"不得用名义背书"），LGPL 要求通知 + 可重链接。
所以声明体系必须是**机器可生成、可复查**的（落到 `LICENSES/` + 审计 JSON），
而不是 README 里手写一段——手写的那段会随依赖变化而失真。

---

## 7. 长期规则（防止复发）

1. **依赖白名单 + 门禁**：`audit_licenses.py` 进 CI；出现 strong-copyleft 或 no-grant 即阻断合入。
2. **新增依赖必须留痕**：许可证 id + 原文进 `LICENSES/`，行进 `THIRD_PARTY_NOTICES.md`。
3. **不从上游复制**：v3.6.2 之后的任何代码、文档、美术、文案都不取；只取**事实**。
   每次从上游获得材料时记录**日期与版本**（证据留存）。
4. **商标不入口**：产品名、广播名、屏显、包装、文件名一律不含 `HaleHound`；
   归属说明里可用（指示性使用）。
5. **发布前跑**：`audit_licenses.py` 与 `collect_licenses.py`，两者结果应稳定。
6. **不重写历史**：上游继承来的 commit 历史是**证据**，`git rebase`/squash 会毁掉它。

---

## 8. 只有 BOSS 能定的事（阻塞项）

| # | 决策 | 为什么卡在这里 |
|---|---|---|
| **D1** | A1/A2/A3 三条依赖：**换库**还是**自写**？ | 决定 2.4GHz、频谱仪、BLE HID 三块的实现方式与工作量；换库快但引入新依赖，自写慢但彻底干净 |
| **D2** | 产品名最终定什么？ | §3 的 T1–T4 改名需要名字；反过来改名不做，商业分发就带商标风险 |
| **D3** | 是否申请自有商标（中国 9/42 类）？ | 决定要不要在设计阶段就把标识固化下来 |
| **D4** | 是否就"MIT 不可撤销性 + 上游专利"取一次外部意见？ | 我无法自查专利；这条影响能否商业化 |
| **D5** | `flash_package/*.bin` 是否现在就从仓库移除？（A5） | 低风险高收益，但涉及删除已提交文件，我不擅自动手 |
| **D6** | `LICENSE` 里的著作权主体写哪个法律实体？ | 我暂填 GitHub 组织名 `RockBase-iot`；出货前应替换为实际主体 |

---

## 9. 本轮未做 / 未验证（诚实标注）

- **未改任何源码**：T1–T7 的改名、A1–A3 的换库都**只出方案，未动手**（D1/D2 未定）。
- **未删除任何文件**：`flash_package/*.bin` 仍在仓库中（D5 未定）。
- **未跑测试**：本轮改动只有文档 + build-reports 工具脚本，不触及 `src/`、`include/`、`test/`，
  故未重跑 hostcheck 与语法检查。
- **未验证**：专利风险、"MIT 不可撤销性"的强度、商标是否已注册——**这三项无法靠仓库自查**。
- **审计的边界**：`audit_licenses.py` 只判定"是否能拿到可识别的许可声明"，不判定
  "声明与实际情况是否相符"（例如 SmartRC 的包内文本与上游声明不一致，脚本只能如实报"不明确"）。
