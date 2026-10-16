# TODO — 可执行任务清单（`nm-cyd-c5`）

| 项 | 值 |
|---|---|
| 生成时间 | 2026-10-16 GMT+8（Track A 于同日第二次重写，依据 `docs/legal/COMPLIANCE_PLAN_zh.md`） |
| 上游证据来源 | `docs/upstream-analysis/USER_EVIDENCE_zh.md`（来源分级 S1 / S2 / S3，含复现命令） |
| 合规依据 | `docs/legal/COMPLIANCE_PLAN_zh.md`（许可链 · 商标分级 · 依赖审计 · 声明体系） |
| 缺口与级别来源 | `REMAINING_WORK.md`（§0.5 可达性走查 · §0.7 U2 落地遗留 · §0.8 外部证据复核 · §1 阻塞项 · §8 合规 · §9 硬件） |
| 本文定位 | `REMAINING_WORK.md` 回答**「还缺什么、多急」**；**本文回答「下一步具体做什么、怎么算做完」** |
| 维护约定 | 每完成一项：在 §9 勾状态并填「证据」列（测试名 / 命令 / 提交 sha），同时更新 `REMAINING_WORK.md` 对应行 |

---

## 0. 怎么用这份清单

每项固定七栏：**ID · 目标 · 为什么（证据）· 动作 · 验收标准 · 依赖 · 规模**。

- **验收标准必须可执行**：能写成断言、命令或一条可复现的观察；不接受「感觉好了」。
- **规模**用 `S` / `M` / `L` 表示相对体量（`S` ≈ 单文件；`M` ≈ 一层；`L` ≈ 跨层），**刻意不给天数** —— 本项目的真实成本由「要不要上机 / 要不要换库」决定，不由行数决定。
- 级别沿用 `REMAINING_WORK.md`：**P0** 阻塞 · **P1** 重要 · **P2** 可延后。
- **ID 前缀**：`C` 合规与许可 · `H` 硬件兼容与共享总线 · `K` 构建·分发·CI · `D` UI/UX · `G` 文档。
  `U*` / `W*` / `R*` / `B*` / `F*` / `T*` / `X*` **沿用既有编号，不重编**。

---

## 1. 里程碑（按「能验证什么」切，不按日期）

| 里程碑 | 达成定义 | 卡在哪 |
|---|---|---|
| **M0 决策闭合** | `C1`（copyleft 二选一）与 `C3`（商标/命名边界）有结论并写进仓库 | **只有 BOSS 能定** |
| **M1 骨架可用** | 上电后每个菜单条目**要么能跑、要么诚实拒绝**；`U3` 已出现在菜单上 | `U3` `U5` `D8` |
| **M2 可上机** | 沙箱外产出 `.bin`；蓝牙菜单在真机上可翻页、可返回、行高可读 | `X1` `X2` `D7` |
| **M3 可发货** | 许可合规 + 第三方声明齐备 + 分区矩阵通过 + 说明书在 | `C1` `C6` `C7` `H4` `G1` |

> `C2`（第三方与衍生声明）已于本轮完成，故从 M3 的卡点中移除；`C6`（移出已提交的 `.bin`）
> 与 `C7`（LGPL 重链接义务）补入 —— 前者是"停止持续分发"，后者是发货的必要条件。

---

## 2. Track A — 合规与许可（**先做决策，后写码**）

> 本节于 2026-10-16 依据 `docs/legal/COMPLIANCE_PLAN_zh.md` 重写。三条与上一版**不同的**结论：
> ① `C1` 的「全 GPL」路线**已被证伪** —— GPL-2.0-only 与 GPL-3.0 互不相容，**不存在**一份能覆盖整体的许可；
> ② 新增一个**独立**阻断项：`ESP32 BLE Keyboard` 完全无授权（原清单只记了它"包内无 LICENSE 文件"，
> 没意识到"无文件"在这里意味着"无授权"）；
> ③ 仓库里 11 个已提交的 `.bin` **本身就是分发行为**，这让 `C1` 从"潜在风险"变成"正在发生"。

### `C1` 按 A 路线替换三项阻断依赖（P0 · M/L）
- **目标**：让「链接出的固件二进制」在许可上成立，判据是 `audit_licenses.py` **退出码为 0**。
  **BOSS 已定 D1 = A 路线（替换）**，方案与验收拆解见 `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`。
- **为什么（S1，可复跑 `python build-reports/audit_licenses.py`）**：
  - `nrf24/RF24@1.4.11` 的 `library.json` 声明 **`GPL-2.0-only`**（`only` = 就是 v2，不含 or-later），随附 LICENSE 为 GPLv2 全文；
  - `kosme/arduinoFFT@2.0.4` 的 LICENSE 正文是 **GPL v3**（`Version 3, 29 June 2007`）；
  - 二者**互不相容**（FSF 相容性清单）⇒ **不存在**任何一份许可能覆盖整个二进制：
    不能按 GPLv2 发（含 v3 部分），不能按 GPLv3 发（v2-only 部分不允许升版），更不能按我们的 MIT 发。
- **动作（A 路线 = 三项，不是两项）** —— 范围**已修正**：原清单只列 RF24 与 arduinoFFT，
  但 `ESP32 BLE Keyboard` 的「无授权」是**独立的**第二个阻断原因，只清 copyleft 一类，二进制依然不可分发：
  1. `arduinoFFT`（GPL-3.0）→ **自写 radix-2**（`include/hound_fft.h`）。**先做**：纯算法、宿主可验、不依赖上机。
     注意两个使用文件用的是**两套 API 风格**（`subghz_attacks.cpp` 数组随调用传入、`wifi_attacks.cpp` 构造时绑定）。
  2. `RF24`（GPL-2.0-only）→ **RadioLib**（MIT，2614★，官方含 nRF24 模块）+ 本仓适配层。
     RadioLib 的 `SPItransfer()` 是 public，四处缺口（const carrier / CRC 长度 / `isPVariant` / `flush_tx`）由我们按数据手册补。
     上游的 `nrf24_raw.h` / `halehound_fft.h` 是**存在性证明，不是可搬运的代码**（专有许可）。
  3. `ESP32 BLE Keyboard`（无授权）→ **自写 HID 层**（耦合面只有 5 个方法）。实测 **NimBLE-Arduino 不提供 HID**，
     故 `nm-cyd-c5` 侧要自己实现 HID over GATT（服务 `0x1812`）；`esp32-cyd` 侧可用 core 的 `BLEHIDDevice`。
     它与 `C3` 的 T1 改名是**同一次改动**，不要分两次做。
- **验收（已修正）**：`python build-reports/audit_licenses.py` **退出码为 0**，即
  `strong copyleft`、`no grant at all`、`unclear grant` **三类同时归零**。
  ~~只看 `strong copyleft` 计数~~ —— 原判据不充分：换掉两个 GPL 而留下无授权的 BLE Keyboard，
  固件仍不可分发。逐项验收标准见方案文档 §2.3 / §3.4 / §4.4。
- **依赖**：无。**结论决定 `F1`/`F3` 的实现方式**（先换库再写码，否则返工）。

### `C2` 第三方与衍生声明 ✅ 本轮已落地（P0 · S）
- **交付物**：`THIRD_PARTY_NOTICES.md`（含 3 个阻断项 + 逐依赖处置）、`NOTICE.md`（派生链 / 修改摘要 / 商标 / 不关联声明）、`LICENSES/`（**14 份逐字副本**）、重写的 `LICENSE`，以及可复跑的 `build-reports/audit_licenses.py` + `build-reports/collect_licenses.py`。
- **5 个「包内无 LICENSE 文件」的依赖已逐个留档**：`rc-switch` / `TinyGPSPlus` → SPDX 官方 LGPL-2.1 原文；`ESP32 BLE Keyboard` → **无授权**占位标记；`SmartRC-CC1101-Driver-Lib` → **自定义宽松许可**（要求指明来源；一手文本副本，非占位）；`TFT_eSPI` → 复合许可原文（`MIT AND BSD-3-Clause`）。
- **验收（已满足）**：清单覆盖 `platformio.ini` 全部 9 项 `lib_deps` + Arduino core 与 IDF 两项传递依赖；`LICENSES/` 内容全部来自一手文件或 SPDX 原文，**无一处誊抄**；`LICENSES/HaleHound-CYD-MIT.txt` 与 `git show 56cf737:LICENSE` **逐字节一致**。

### `C3` 商标与命名边界（P0 · S · **需 BOSS 定名**）
- **为什么**：MIT **不授予商标权**（通篇未提商标，沉默≠授权）；上游用同一牌子卖**同类**商品（固件 + 该固件的硬件在售）⇒ 混淆风险是实质性的。已核实的越界点按风险排序：
  1. `src/legacy/bluetooth_attacks.cpp:9153`（根目录副本同在）—— `new BleKeyboard("HaleHound KB", "HaleHound", 100)`：**设备对附近所有手机广播商标名**（最高）；
  2. `src/app/LegacyApp.inc:2782 / 4186 / 4199` —— `drawGlitchTitle(..., "HALEHOUND")`：屏幕品牌字；
  3. `include/icon.h:1460-1464` —— `bitmap_halehound_splash[]`：上游**美术资产**编进固件；
  4. `src/app/LegacyApp.inc:2527 / 2579` —— `tft.print("By: HaleHound (JMFH)")`：须改成归属声明形态；
  5. `flash_package/` + `dist/` 的 11 个 `HaleHound-*.bin` **文件名**（与 `C6` 同一批处理）；
  6. `README.md` / `LICENSE` 的项目层品牌；仓库名 `HaleHound-CYD`（属 fork 描述性命名，可暂留，但要与产品名切开）；
  7. 85 个文件共 **4359 处**，绝大多数是颜色宏 `HALEHOUND_MAGENTA` / `HOTPINK` / `GUNMETAL` / `DARK`（内部标识符，低风险，机械重命名）。
- **动作**：按上表产出「保留（指示性使用）/ 替换（自有品牌）」清单并执行；归属行统一为 `Based on HaleHound-CYD by Jesse C. Hale (MIT)`，另附不关联声明。
- **验收**：`grep -rn "HaleHound" src/ include/` 只剩归属声明那几处；对外产物文件名、屏显文字、广播名不含该词。
- **依赖**：**先定产品名**（§8 `D2`）。

### `C4` 不搬运 v3.6.2+ 的专有物料（P0 · S）
- **为什么（已更新）**：上游换证发生在 **v3.5.0 → v3.6.2 之间**（逐 tag 抓 LICENSE 全文比对确认：v3.5.0 = MIT 14991 字节，v3.6.2 = 专有 16575 字节）；v4.0 的 SCOPE 明确含 `Documentation and README files` 与 `ASCII art and branding assets`。我们分叉于 2026-03-16（v3.4.0 之后），**在换证之前**。
- **动作**：文档与 `dist/` 逐文件标注来源与许可；对外销售物料只用功能性描述（功能名、菜单结构属功能性信息）。总则：**事实可读，表达不可抄**。
- **验收**：审计表落库；`dist/` 内每个上游原文文件都有来源标注。
- **依赖**：`C3`。

### `C5` 保持「上游行为 = 发布方自述」口径（P2 · S）
- **验收**：`RELEASE_BRIEF_zh.md` 与 `USER_EVIDENCE_zh.md` 中每条上游行为都带来源级标记，**无一处被表述为「已验证」**。将来若拿到真机行为，回填为 S1。

### `C6` 把已提交的 `.bin` 移出仓库（P0 · S · **止血，可今天做完**）
- **为什么**：`flash_package/*.bin`（10 个）与 `dist/*.bin` 共 **11 个、18.8 MB** 已在 git 中；`dist/` 虽在 `.gitignore`，但文件在加那条 ignore **之前**就已提交，仍被跟踪。**仓库公开 = 正在分发**，而这些二进制同时带着 `C1` 的不可分发组合、`C3` 的商标文件名。不做这步，`C1`/`C3` 的整个周期里仓库都处于「持续再分发」状态。
- **动作**：`git rm --cached flash_package/*.bin`；`.gitignore` 补 `flash_package/*.bin`（`dist/` 已有）。
- **验收**：`git ls-files | grep -c '\.bin$'` 归零；`.gitignore` 同时覆盖两条路径；权威产物改由发布页 / `scripts/package_firmware.ps1` 提供。
- **依赖**：无。**但涉及移除已提交文件，等 BOSS 确认**（§8 `D5`）。

### `C7` 履行 LGPL-2.1 的静态链接义务（P0 · S）
- **为什么**：Arduino core、ESP-IDF 预编译库、`rc-switch`、`TinyGPSPlus` 均为 **LGPL-2.1-or-later**（core 与 IDF 由 `package.json` 声明，另两个由源文件头声明）。LGPL **允许**静态链接，但要求接收方**能够重新链接**到修改后的库版本。
- **动作**：① `LICENSES/` 已备（含 LGPL-2.1 原文）；② 写一节「如何重新链接」的构建说明（工具链版本 + 完整 flag，可从 `build-reports/_syntax_*.rsp` 取真值）；③ 声明按请求提供 `.o`；④ 或把 `rc-switch` / `TinyGPSPlus` 换成自写实现以缩小义务面。
- **验收**：文档里有可复现的重链接步骤，照文档能由公开源重建出等价固件。

### `C8` 逐文件 SPDX 头（P1 · S/M）
- **为什么**：MIT/BSD 的归属义务是**分发条件**，目前只有仓库级声明。政策已定于 `docs/legal/COMPLIANCE_PLAN_zh.md §6.2`。
- **动作**：自写文件写 `SPDX-License-Identifier: MIT` + 我方 `SPDX-FileCopyrightText`；**改自上游的文件必须保留上游著作权行**；第三方原样引入的文件不动。
- **验收**：可复查脚本 —— `src/`、`include/` 下每个文件含 SPDX 行，或出现在显式豁免清单里。
- **注**：**单独一次提交**，避免千行 diff 淹没许可改动。

### `C9` 依赖许可门禁进 CI（P1 · S）
- **为什么**：`audit_licenses.py` 现在**有阻断项就 `exit 1`**，可直接当门禁；但没接进任何流程，靠人记得跑就等于没有。
- **动作**：接入 `K2` 的 CI 矩阵；新增依赖必须同时更新 `LICENSES/` 与 `THIRD_PARTY_NOTICES.md`。
- **验收**：故意加一个 GPL 依赖时 CI 变红，且红灯输出能直接定位到依赖名与许可。

### `C10` SmartRC 许可澄清 ✅ 本轮已落地（P1 · S）
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

---

## 3. Track B — 硬件兼容与共享总线

### `H1` 共享 VSPI 总线仲裁层（P0 · L）
- **为什么**：SD + CC1101 + NRF24 + PN532 全挂 GPIO 18/19/23；上游讨论最热的两个 issue 正是这层缺失的后果（**#165 九条评论 / #168 七条**：「E01、E07、PN532 都接上后 SD 卡检测不到」）。
- **动作**：引入单一「总线持有者」抽象：CS 独占 + 传输互斥 + 切换时 SD 重挂载；所有射频驱动经它取权。
- **验收（可断言）**：① 宿主测试用假总线断言「任一时刻至多一个 CS 为低」② 非法并发调用序列必须被**拒绝并给出可读理由** ③ 切换压力测试后 SD 仍可读（`SdCode::Ready` / `Remounted`，不得 `Busy` / `NotMounted`，与 `T1` 同口径）。
- **依赖**：无。**`F1`/`F2`/`F3` 依赖它**。

### `H2` PN532 总线决策（P0 · S 决策 + M 实现）
- **为什么**：上游/社区标准是 **SPI**（`SS=GPIO17`，DIP 设 SPI；社区手册同表），本 fork `WirePn532Link` 只有 **I2C**（`Wire.begin` + 100 kHz + ready byte `0x01`）。**PN532 的 SPI 与 I2C 帧格式不同**，不是换一行 `Wire` 的事。
- **动作**（二选一）：① **保持 I2C** → 文档显式写出与上游接线的差异（否则用户照官方图接线必失败）；② **兼容 SPI** → 按 PN532 SPI 帧格式新增一条 `IRfidTransport` 实现。
- **验收**：决策落库；选①则 `docs/` 有接线差异表 + 引脚图；选②则新链路有宿主测试（假 SPI 链路 + 帧往返），且 `test_pn532_transport` 仍全绿。
- **依赖**：与 `U4` 是**同一次决策**（别开两张单）。

### `H3` 射频引脚可重指派（P1 · M）
- **为什么**：上游 #179 的功能请求（「可做成几乎免焊的构型」）；且本 fork 接线已与上游不同，硬编码引脚会持续制造分叉。
- **动作**：引脚从编译期 `-D` 常量改为运行期配置（SD / EEPROM），带默认值兜底。
- **验收**：换一套引脚配置后 `Radio Test` 自检通过；冲突/非法引脚在菜单里给出拒绝理由；配置读写有宿主测试。

### `H4` 闪存 / 分区矩阵（P1 · M）
- **为什么**：`nm-cyd-c5` 用 `PART_16MB=1`；上游存在「16 MB CYD 装不上」（#167）。
- **动作**：4 MB 与 16 MB 各出一份分区表；`T5` 的 `FLASH_LAYOUT` 检查覆盖两者。
- **验收**：两种分区都能产出 `.bin` 且 `FLASH_LAYOUT` PASS；#167 的场景写成回归用例，记录进 `build-reports/`。

### `H5` 供电巡检与 brownout 对策（P1 · S）
- **为什么**：上游 #182（空板开机即 brownout 重启循环、屏不亮）；上游 README 把「Ebyte 模块必须独立 5V→3.3V 供电」写成硬要求。
- **动作**：射频初始化失败时给出**指向供电的可操作提示**（屏上 + 串口各一行），而不是静默失败。
- **验收**：故意不给外设供电时，屏幕给出明确提示且**不进入无日志卡死**；该场景的启动日志片段存入 `build-reports/`。

---

## 4. Track C — 可达性与接线收尾（沿用既有 ID，只补「验收标准」）

| ID | 一句话 | 验收标准（可执行） |
|---|---|---|
| **`U3`** | `ChameleonUltra` 补 `FeatureId` + descriptor | `test_feature_menu_model` 断言蓝牙菜单**包含**该条目；点进去不再是「catalog 里没有它」 |
| **`U4`** | `Pn532I2cDriver` 去留（与 `H2` 同一次决策） | 决策落库；若删除，`grep -rn Pn532I2cDriver src/ include/` 为空且两端语法检查仍绿 |
| **`U5`** | 16 条空壳：14 条按上游 Settings(9)/Tools(5) 实做，2 条（`SubGhzSavedProfiles`/`SigintWardriving`）摘掉或补实现 | 菜单里不存在「点进去无反应」的条目 —— 要么有实现，要么有诚实拒绝屏（`D8`） |
| **`W1`** | 新模型/驱动注册进 `FeatureCatalog` | 每个注册项都有 `FeatureId` 与 factory；「菜单 == catalog」宿主测试全绿 |
| **`W2`** | `CardDumpFormat` → `StorageService` | 落盘后可读回并逐字节比对；路径全部经 `PathPolicy` |
| **`W3`** | `LegacyApp.inc` 分域切分（Tools/Settings 优先） | 切分后宿主全量与两端语法检查仍绿；`LegacyApp.cpp` 行数显著下降 |
| **`W6`** | 收并旧 `Pn532I2cDriver` | 同 `U4` |
| **`X1`** | 沙箱外 `pio run` 产出 `.bin` | `scripts/package_firmware.ps1` 的 `BUILD_FRESHNESS` / `FLASH_LAYOUT` / `PACKAGE_FIRMWARE` 三项 PASS |
| **`X2`** | 两台实机上机（B1–B4 采样） | BLE **地址字节序反转**与 **NimBLE live 扫描前置条件**两条结论在真机上被证实或推翻，并回填文档 |

---

## 5. Track D — UI/UX

### `D4` Loot 条目可删除（P1 · S/M）
- **为什么**：上游 #184「Unable to delete credentials from Loot」。
- **验收**：可删单条与整类；删除后重新进入列表**不复活**；空状态显示为空而不是报错（与 `T1` 的「不得空目录」区分：这是删除后的**正常**空）。

### `D5` GPS 屏：冷启动状态与等待提示（P1 · S/M）
- **为什么**：上游 #162（**13 条评论**）—— 用户不知道冷启动要等多久。
- **验收**：未定位时显示卫星数 / 信噪 / 已等待时长，并给出「首次定位可能需要数分钟」的说明；定位后切换为坐标视图。

### `D6` 电池电量显示（P2 · S）
- **为什么**：上游 #173 请求 fuel gauge；本 fork 的 NM 产品需求里也有「电量仅屏幕显示」。
- **验收**：屏幕上有电量指示；**无电量采样硬件时显示「未知」而不是编一个百分比**。

### `D7` 菜单分页 / 滚动（P1 · M）
- **为什么**：本 fork 蓝牙菜单 10 → 15 行后竖屏行高 **25 → 16 px**（`REMAINING_WORK §0.7` 实测），这是本 fork 相对上游**唯一变差**的地方；上游 #187 恰是同类导航缺陷。
- **动作**：照 `GattTreeView` 的分页做法（光标跟随页）。
- **验收**：竖屏 15+1 行时行高回到可读阈值（≥20 px）或分页生效；翻页 / 返回 / 首末项切换可在宿主测试中断言。

### `D8` 空壳条目的诚实拒绝屏（P1 · S）
- **为什么**：`REMAINING_WORK §0.5 A10` + 上游 #183（Tools/Skimmer 冻结且**串口无日志**）—— 「点了没反应」是最坏的一种失败。
- **验收**：任何未实现条目进入后 1 次 tick 内给出屏上说明 + 串口一行；不出现白屏、不出现无输出卡死。

### `D9` `MenuLayout` 密度回归测试（P1 · S）
- **为什么**：§0.7 的行高结论是**一次性临时测量**（当时脚本已删），不写进测试就会再次漂移。
- **验收**：新增宿主套件，对 `240x320` / `320x240` × 条目数 `8/12/15/16/24` 断言「`valid()` 为真、行高有下界、末行 bottom 不越界」；阈值必须来自 `MenuLayout` 自身语义，不得硬编码猜测值。

### `D10` 其余 8 个子菜单改 catalog 驱动（P1 · M）
- **为什么**：`REMAINING_WORK §0.7` 遗留 —— 蓝牙那支已改，其余 8 支（WiFi / 2.4GHz / SubGHz / RFID / Jam Detect / SIGINT / Tools / Setting）仍手维护，与 catalog 的漂移**依然存在**。
- **验收**：`grep` 断言各子菜单条目来自 `FeatureMenuModel`；「菜单 == catalog」的宿主测试覆盖**全部** category。

---

## 6. Track K — 构建 · 分发 · CI

| ID | 一句话 | 验收标准（可执行） |
|---|---|---|
| **`K1`** | `.bin` 一致性：`PART_16MB` 与 4 MB 两种产物 | 两种产物都能生成、尺寸在预算内、SHA256 留档进 `build-reports/` |
| **`K2`** | CI 构建矩阵（`W5` 的落地形态） | 每次推送自动跑：两端语法检查 + 宿主全量 + 分区矩阵 + **`C9` 的许可审计**；红灯能定位到文件 |
| **`K3`** | 版本号单一来源（`W4`） | 固件 / 菜单 About / 构建产物 / README 四处版本号来自同一处，并有断言防漂移 |
| **`K4`** | 固件体积预算 | 超阈值即 fail，并输出各模块占用 |
| **`K5`** | OTA 路径验证（`/firmware`） | SD 内 `.bin` 能被识别并校验（`/firmware` 已在 `PathPolicy` 定义） |

---

## 7. Track G — 文档

| ID | 一句话 | 验收标准（可执行） |
|---|---|---|
| **`G1`** | 「刷完之后怎么用」说明书 | 覆盖上游明确缺失的四件事：**串口怎么用**（#174）、**GPS 要等多久**（#162）、**Loot 怎么删**（#184）、以及**本 fork 与上游的接线差异**（PN532 总线 / DIP，含官方与社区措辞冲突，见 `USER_EVIDENCE_zh.md §3.4`） |
| **`G2`** | SD 路径契约回归 | 「8 条路径与上游一致」这件事由测试锁定，改动即红 |
| **`G3`** | 文档卫生：`_gap_a.md` / `_gap_b.md` 改名 | 名字不再暗示「差距分析」（实为 release 原文抓取），或至少头部加一行说明 |

---

## 8. 明确不做（防止清单膨胀）

| 项 | 理由 |
|---|---|
| 从上游「再取代码」 | 上游 v3.6.2 起为**专有**（禁止衍生 / 再分发 / 改署名）。**口径更正**：上游当前 `main` 确实只有 README + LICENSE，但**它的历史曾有源码**，而我们这份 fork 已于 2026-03-16（换证前）拿到那份 MIT 源码；且上游改写过历史，两边 `git merge-base` 为空 ⇒ 既无法「同步」，也不需要「按文档独立实现」来替代 —— **我们缺的只是 v3.6.2 之后的新功能，那部分只能自己写**（`C4`） |
| 发布上游 `.bin` / 镜像 | 上游许可禁止再分发与镜像，且我们也没有其二进制 |
| 用 `HaleHound™` 做本 fork 的产品名 | 商标边界（`C3`） |
| 把整个固件改为 GPL 后分发 | **已证伪**：GPL-2.0-only 与 GPL-3.0 互不相容，这条路物理上不存在（`C1`） |
| 追求「条目数对齐上游 v4.0」 | 上游 v4.0 的 Drone RID Spoof 上线即崩（#189 未关闭）—— 条目数 ≠ 成熟度 |
| 嵌套 / 硬嵌套 MIFARE 攻击 | `REMAINING_WORK §2 R7` 已定为「只做诚实报告，不做攻击」 |

---

## 9. 状态跟踪

| ID | 状态 | 证据（测试名 / 命令 / sha） |
|---|---|---|
| `C1` | 🟡 A 路线已定，代码未改 | `audit_licenses.py` 当前输出 `strong copyleft: 2` + `no grant at all: 1`，exit=1；方案见 `docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`（三项、分三阶段） |
| `C2` | ✅ 已完成 | `THIRD_PARTY_NOTICES.md` / `NOTICE.md` / `LICENSES/`（14 份）/ `LICENSE`；`audit_licenses.py` + `collect_licenses.py` 可复跑；`LICENSES/HaleHound-CYD-MIT.txt` 与 `git show 56cf737:LICENSE` 逐字节一致 |
| `C3` | ⬜ 未开始（等命名） | 越界点 T1–T7 已落 `docs/legal/COMPLIANCE_PLAN_zh.md §3.2` |
| `C4` | 🟡 部分 | 换证时间点已钉死（v3.5.0 MIT → v3.6.2 专有）；逐文件来源标注未做 |
| `C5` | ⬜ 未开始 | |
| `C6` | ⬜ 未开始（等 BOSS 确认） | `git ls-files \| grep -c '\.bin$'` 当前为 **11** |
| `C7` | 🟡 部分 | `LICENSES/` 已备 LGPL-2.1-or-later 原文；**标识符已修正为 or-later**（原报裸 `LGPL-2.1`，含义是 -only）；「如何重新链接」说明未写 |
| `C8` | ⬜ 未开始 | |
| `C9` | ⬜ 未开始 | `audit_licenses.py` 已具备 exit 1 语义，未接 CI |
| `C10` | ✅ 已完成 | 分类改 `bespoke-permissive`（SPDX `LicenseRef-SmartRC-bespoke-permissive`）；`LICENSES/SmartRC-CC1101-Driver-Lib-bespoke-permissive.txt` 与包内 `LICENSE.txt` `cmp` 一致；`unclear grant: 0` |
| `H1` | ⬜ 未开始 | |
| `H2` | ⬜ 未开始 | |
| `H3` | ⬜ 未开始 | |
| `H4` | ⬜ 未开始 | |
| `H5` | ⬜ 未开始 | |
| `U3` | ⬜ 未开始 | |
| `U4`/`W6` | ⬜ 未开始 | |
| `U5` | ⬜ 未开始 | |
| `W1` | ⬜ 未开始 | |
| `W2` | ⬜ 未开始 | |
| `W3` | ⬜ 未开始 | |
| `X1` | 🟡 等 BOSS 在沙箱外执行 | `-fsyntax-only` 只证编译，未链接 |
| `X2` | 🟡 等实机 | |
| `D4`–`D10` | ⬜ 未开始 | |
| `K1`–`K5` | ⬜ 未开始 | |
| `G1`–`G3` | ⬜ 未开始 | |

---

## 10. 只有 BOSS 能定的决策（阻塞 Track A）

| # | 决策 | 为什么卡在这里 |
|---|---|---|
| **D1** | ~~两条依赖换库还是自写~~ **已定：A 路线（替换，三项）** | 2026-10-16 BOSS 决定。范围由两项修正为三项（含无授权的 BLE Keyboard）；混用策略：FFT 自写、RF24 用 RadioLib、BLE HID 自写 |
| **D2** | 产品名最终定什么？ | `C3` 的 T1–T4 改名需要名字；不改名则商业分发带商标风险 |
| **D3** | 是否为自有品牌申请商标？ | 决定要不要在设计阶段就固化标识 |
| **D4** | 是否就「MIT 不可撤销性 + 上游专利」取一次外部法律意见？ | 仓库无法自查专利；这条影响能否商业化 |
| **D5** | `flash_package/*.bin` 与 `dist/*.bin` 是否现在移出仓库？（`C6`） | 低风险高收益，但涉及移除已提交文件 |
| **D6** | `LICENSE` 的著作权主体写哪个法律实体？ | 现暂填 GitHub 组织名 `RockBase-iot`，出货前应替换为实际主体 |

---

**生成依据**：`docs/upstream-analysis/USER_EVIDENCE_zh.md`（外部取证 + 来源分级）、`docs/legal/COMPLIANCE_PLAN_zh.md`（许可链 / 商标 / 依赖审计）与 `REMAINING_WORK.md`（§0.5 / §0.7 / §0.8 / §8 / §9）。
**本文只写「做什么、怎么算做完」；级别与缺口定义以 `REMAINING_WORK.md` 为准。**
