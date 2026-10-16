# 上游使用说明 · 许可 · 社区反馈取证（HaleHound-CYD）

| 项 | 值 |
|---|---|
| 取证时间 | 2026-10-16 GMT+8 |
| 上游仓库 | `JesseCHale/HaleHound-CYD` |
| 本 fork | `RockBase-iot/HaleHound-CYD`（分支 `nm-cyd-c5`） |
| 用途 | 给 `REMAINING_WORK.md` / `TODO.md` 提供外部依据：**上游怎么用、许可允许什么、社区实际在抱怨什么** |
| 抓取方式 | `curl`（raw 文件 / GitHub REST / 官网 CDN）+ 检索式 `WebFetch`/`WebSearch`；命令见 §11 |
| 原始件 | 不入库，落在工作区外 `tmp-research/`：`hh_readme.md`(29 931 B)、`LICENSE.txt`(16 616 B)、`releases.json`(365 930 B)、`hh_build_guide_v141.pdf`(9.15 MB)、`hh_build_guide.txt` |

**来源分级**（沿用本仓库证据门口径）

| 级 | 含义 | 本文用途 |
|---|---|---|
| **S1** | 一手：上游仓库原文 / 上游 API 元数据 / 上游 issue 正文 / 本仓库本地文件 | 可作结论依据 |
| **S2** | 第三方实测：社区作者手册、第三方构建者博客、其它固件的对照 issue | 可作推论依据，需注明 |
| **S3** | 二手或机器生成：AI 摘要站、SEO 评测站 | **只用于判断公众认知，不作技术依据** |

---

## 1. 一屏结论

1. **上游不是开源项目，而且许可是「最近才收紧」的。** 上游 v4.0 的 `LICENSE` 是 *HALEHOUND-CYD PROPRIETARY LICENSE*（`Copyright (c) 2026 Jesse C. Hale. All rights reserved.`），逐条禁止：再分发二进制（除本仓库 / `flash.halehound.com`）、反向工程、**由二进制衍生**、商业分发、镜像托管、**移除或改动任何 branding / attribution / license 声明**、**声称作者身份**。仓库里只有 `README.md` + `LICENSE` + `.github/FUNDING.yml`：**没有源码，也没有 `.bin`**（28 个 release 的资产只有 5 张宣传图）。**但本 fork 的 `LICENSE` 是 MIT** —— 即上游在本 fork 的基线 `v3.4.0` 之后换过证（§4）。
2. **本 fork 的依赖集里有两个 GPL 库，其中一个与另一个互不相容。** `nrf24/RF24@1.4.11` 的 `library.json` 声明 `"license": "GPL-2.0-only"`；`kosme/arduinoFFT@2.0.4` 的 LICENSE 正文是 GPL v3。上游**刻意避开**了这一点（自写 clean-room `nrf24_raw.h` / `halehound_fft.h`，LICENSE 里明写 "This project contains zero GPL-licensed code"）。⟹ **当前依赖集产出的二进制在许可上不可分发**，GPL-2.0-only 与 GPL-3.0 也不能共处一个作品（§5）。
3. **上游的 PN532 走 SPI，本 fork 的 RFID 栈只有 I2C。** 官方接线：`SS=GPIO17`、DIP 设 SPI；社区手册同表（MOSI/MISO/SCK = IO23/19/18、SS = IO17）。本 fork `WirePn532Link` 是 `Wire.begin` + 100 kHz + I2C ready byte `0x01`，全仓 PN532 相关文件 SPI 计数为 0（只有 `St25r3916Transport` 是 SPI）。⟹ **本 fork 驱动不了上游/社区标准的 PN532 接线**，这是硬件层分歧，不是配置项（§6、§9）。
4. **社区最集中的抱怨恰好是本 fork 已经当成硬约束的三件事**：共享 VSPI 下的 SD 识别失败、CC1101/SubGHz 冻结、Settings/Tools 无响应。前两类是**总线仲裁**与**看门狗**问题，第三类是**空壳菜单**问题。
5. **上游 issue #187「蓝牙子菜单没有返回主菜单」在本 fork 已经解决** —— 菜单构建器会追加 `Back to Main Menu`（`LegacyApp.inc:441-445`）。这是可直接对外说的差异点，但**必须同时说清行高退化**（10→15 行后竖屏 25→16 px）。
6. **公开评测普遍过时且互相矛盾**（称最新为 v3.6.2 / v3.7.2、star 1368 / 1400），实测是 **v4.0 / 1812 stars / 139 forks / 3 open issues**。这些站只能用来确认公众认知锚点（「$15 级多协议工具」「Pwnagotchi 替代品」），不能当技术依据（§8）。

---

## 2. 上游仓库事实（S1，REST API 可复跑）

| 字段 | 值 |
|---|---|
| stars / forks / watchers | **1812 / 139 / 64** |
| open issues | **3** |
| 创建 / 最后推送 | 2026-02-17 / 2026-10-15 |
| default branch | `main` |
| license (GitHub 判定) | **NOASSERTION**，显示为 "Other" → GitHub 无法识别（因为它是自制专有文本） |
| description | `ESP32-DIV HaleHound Edition for Cheap Yellow Display - Multi-protocol offensive security toolkit` |
| topics | `bluetooth, cc1101, cheap-yellow-display, cyd, **defcon, defcon33, defcon34**, esp32, hacking, nrf24l01, offensive-security, pentest, security, subghz, wifi` |
| 根目录内容 | `.github/FUNDING.yml`、`LICENSE`(16 616 B)、`README.md`(29 931 B) —— **仅此三项** |
| releases | 28 个（v4.0 = 2026-10-15） |
| release 资产 | **仅 5 个，全是宣传图**：v3.9.1 的 4 张 `nrf-jam-*.jpg`（4.8k–6.3k 次下载）、v3.5.5 的 `halehound-v355-release.png`（**22 579** 次）。v4.0 **零资产** |

> **两点修正既有认知**：① 上游 release 并非「全零资产」，但确实**从未附带固件**；② `_gap_a.md` / `_gap_b.md` 的文件名像「差距分析」，实际内容是 raw release 全文抓取，容易被误读成结论（见 §10 文档卫生）。

**唯一分发通道**：网页刷写器 `flash.halehound.com`（需桌面版 Chrome/Edge；Safari 与手机不行）。

---

## 3. 上游使用说明书精读（S1，`README.md` 29 931 B 全文）

上游 README 就是完整说明书：板型、硬件、菜单树、接线、SD 布局、刷写、编译、功率、Known Issues 全在里面。

### 3.1 板型矩阵：上游 4 个目标 vs 本 fork 2 个

| 上游板型 | 构建目标 | 显示 | 状态 | 本 fork 是否有 |
|---|---|---|---|---|
| ESP32-2432S028 (2.8") | `esp32-cyd` | ILI9341 240x320 | Fully Tested | ✅ 有（Bluedroid / Arduino 2.0.17） |
| QDtech E32R35T (3.5") | `esp32-e32r35t` | ST7796 320x480 | Fully Tested | ❌ 无 |
| QDtech E32R28T (2.8") | `esp32-e32r28t` | ILI9341 240x320 | Fully Tested | ❌ 无 |
| NM-RF-Hat (2.8") | `esp32-cyd-hat` | ILI9341 240x320 | Supported | ❌ 无（但本 fork 有 `nm-cyd-c5`） |

本 fork 的 `platformio.ini` 只有 `[env:native]` / `[env:esp32-cyd]` / `[env:nm-cyd-c5]`。⚠️ **本 fork 的主板不是上面任何一个**：`nm-cyd-c5` 用 `ST7789_DRIVER=1`、`TFT_BL=25`、`TFT_DC=24`、`ROTATION=1`、`PART_16MB=1`、`BOARD_HAS_PSRAM=1` —— 而 **ST7789 正是上游 issue #169 里被讲成「另一类 CYD 驱动」的东西**。面板不同 ⟹ 颜色顺序、反色、旋转、行高这条链不能照抄上游结论。

### 3.2 硬件与供电（上游把它写成硬要求）

必需：CYD 板 + CC1101（HW-863 或 E07-433M20S）+ NRF24L01+PA+LNA + **PN532 V3（Elechouse，SPI 模式）** + GPS（GT-U7 / NEO-6M）。
可选：E07-433M20S PA、10 µF 电容（NRF24 抗复位）、FAT32 microSD、LiPo + boost。

> **独立 3.3 V 供电是硬要求**：Ebyte PA 模块（E07-433M20S / E01-2G4M27SX）**必须**用独立 5V→3.3V buck 供电（AMS1117-3.3 / MP2307，≥500 mA），不能共用 CYD 的 3.3 V 轨，否则 brownout、随机复位、射频初始化失败。信号线仍直连 GPIO，只有 VCC 独立。

### 3.3 菜单树（v4.0，按域计数）

WiFi 8 · Bluetooth 10+5（Lunatic Fringe 子组）· 2.4GHz 7 · SubGHz 7 · RFID 6 + 后端选择器（PN-532 / Chameleon Ultra / ST25R3916 *stub*）· Jam Detect 4 · SIGINT 6 · Tools 5 · Settings 9 · About 1 · Drone 4 · Skimmer 1。

对本 fork 的直接映射：**Settings 9 项**与**Tools 5 项**正好等于 `REMAINING_WORK §0.5 A10` 里那 16 条空壳中的 14 条（另 2 条是 `SubGhzSavedProfiles`、`SigintWardriving`）。也就是说 **A10 不是「多出来的空壳」，而是上游菜单里真实存在的 14 项还没落地** —— 优先级判断要按这个来。

Settings 9 项明细（上游原文）：Brightness(10-255) · Screen Timeout(30s/1m/2m/5m/10m/Never) · Swap Colors(BGR↔RGB) · Invert Display · Color Mode(Default/Colorblind/High Contrast) · Rotation(0°/180°/90°CW/90°CCW) · Device Info(Heap/CPU/flash/board) · **Set PIN（4 位开机锁）** · CC1101 Module(HW-863 ↔ E07 PA)。

### 3.4 接线（三处与本 fork 相关）

| 模块 | 上游官方 | 社区手册（S2） |
|---|---|---|
| CC1101 | SCK/MOSI/MISO = 18/23/19；**CS = 27**（CYD 2.8"，`CN1`）；GDO0(TX)=22；GDO2(RX)=35。E32R35T 的 CS 移到 **21**（27 是 3.5" 背光） | 同，另注 OG CYD 非 SPI 版：CC1101 CS=27、NRF CS=4、CC1101 TX=26、RX=0 |
| NRF24 | CSN=4（原 RGB R）、CE=16（原 RGB G）、IRQ=17（原 RGB B） | 同；SPI CYD 上 CSN 移 **26** |
| **PN532** | **SPI**：SCK/MOSI/MISO = 18/23/19，**SS = GPIO 17**；DIP **CH1=OFF, CH2=ON** | **SPI**：MOSI=IO23、MISO=IO19、SCK=IO18、**SS=IO17**；DIP「下拨右、上拨左」 |
| GPS | **TX → GPIO 1**（P1 座），RX 不用；固件自动扫 GPIO 1/3/26；**运行时 `Serial.end()` 让出 UART0**，退出恢复；9600 波特 | 手册同（另有 ATGM336h 变体） |

**⚠️ 已发现的一处文档冲突**：官方 README 说 PN532「CH1=OFF, CH2=ON」，社区手册说「下拨右、上拨左」。两者很可能指同一物理设置的不同标注习惯，但**对外就是混淆源**，本 fork 若写自己的说明书必须给出自己那张照片/明确标注。

**共享 VSPI**：所有外设（含内置 SD）共用 GPIO 18/19/23，上游声明「同一时刻只有一个设备说话 —— SPI manager 自动仲裁」。本 fork 目前**没有**这类仲裁层（见 §7）。

### 3.5 SD 布局（上游 8 项 ⟷ 本 fork `PathPolicy`）

| 上游 | 本 fork | 一致 |
|---|---|---|
| `/subghz/` `.sub` 文件 | `/subghz` | ✅ |
| `/eapol/` 握手/PMKID | `/eapol` | ✅ |
| `/wardriving/` GPS 日志 | `/wardriving` | ✅ |
| `/wp_loot/` WhisperPair/Predator/FlockYou | `/wp_loot` | ✅ |
| `/loot/` Predator GATT dump | `/loot` | ✅ |
| `/creds.txt` IoT Recon 自定义凭据 | `/creds.txt` | ✅ |
| `/iot_recon.txt` | `/iot_recon.txt` | ✅ |
| `/firmware/` OTA | `/firmware` | ✅ |

本 fork 额外有 `/loot/credentials`（`PathPolicy.cpp:20`）。⟹ **SD 契约是与上游对齐的**，这是好消息，应在 TODO 里加一条「锁定该契约」的测试（防止后续漂移）。

### 3.6 首启 / 常见问题 / 功率

- **首启自动进触摸校准**（点 4 个角）；方向不对去 Settings > Rotation，不用重刷。
- **CH340 驱动**：CYD 用 CH340 USB 串口（Win/macOS 需装，Linux 内核 5.x+ 自带）。
- 官方 Known Issues 5 条：NRF24 随机复位→加 10 µF；GPS 与 USB 共用 GPIO3→固件已处理；刷后显示倒置→Rotation；触摸偏移→首启自动校准或 Tools > Touch Calibrate；Python 3.14 破坏 PlatformIO→用 3.10–3.13。
- **TX 功率表**：WiFi +20.5 dBm · NRF24+PA+LNA +20 dBm · CC1101 +12 dBm（stock）/ +20 dBm（E07 PA）· BLE +9 dBm。⟹ 本 fork 的 `RadioTuningModel` 应能对上这 4 个数（可作验收断言）。
- 编译：`pio run -e <board> --target upload`；串口 `pio device monitor -b 115200`；**要求 Python 3.10–3.13**。

### 3.7 商业面（它会改变「对标」的含义）

上游自带商店：`halehound.com`（Shopify）。在售：**2.8" CYD E32R28T Exposed SPI $24**、**3.5" CYD Exposed SPI $27**、**3.5" 外壳 $45**、**4 件套外壳 $45**；另有周边站 `merch.halehound.com`。官网自称「40+ Attack Modules / 5 Radio Protocols / 4 Supported Boards / **0 Safety Nets**」，社区入口：GitHub、YouTube `@HaleHound07`、TikTok `@halehound7`、以及作者自己的演示视频（README 末尾链接 `youtube.com/watch?v=25et7KJzb1s`）。

⟹ 对本 fork 的含义：上游卖**硬件 + 预刷固件**。本 fork 若要做自己的产品（NM 系列），**商标与固件署名的边界必须显式决定**（§4.3）。

---

## 4. 许可与再分发（S1）

### 4.1 本 fork 自己的 LICENSE 是什么

`LICENSE`（15 316 B）标题是 **MIT LICENSE**，双著作权行：

```
Copyright (c) 2026 Jesse C. Hale (HaleHound-CYD Edition)
Copyright (c) 2023 CiferTech (original ESP32-DIV)
```

即本 fork 拿到的授权是**上游 MIT 时代的授权**（对应基线 `v3.4.0`）。该文件还含：

- **SCOPE**：「applies to all contents of this repository」，明列 `.cpp/.h/.ino`、构建脚本、文档、**引脚映射**、**菜单定义与 UI 资产**、ASCII art / branding。
- **ATTRIBUTION REQUIREMENTS**：直接使用需「retain this LICENSE / maintain copyright notices」；**修改与 fork** 需「include original copyright notice / clearly indicate modifications」，并给出建议写法 `"Based on HaleHound-CYD by Jesse C. Hale"`；衍生作品「may create derivative works under the same MIT license」，须保留归属链。
- **THIRD-PARTY COMPONENTS**：列了 ESP32 Arduino Core(LGPL)、TFT_eSPI、RCSwitch、**RF24**、TinyGPSPlus、ArduinoJson、SmartRC-CC1101，然后一句「Users are responsible for compliance with all applicable third-party licenses.」—— **列了 RF24 却没写它是 GPL-2.0-only**（§5 的问题就出在这里）。
- **DISCLAIMER & RESPONSIBLE USE**：5 条允许用途 + 6 条禁止用途（与上游同源文本）。

**本 fork README 的署名状态（已核对）**：保留了 `By [JesseCHale]`、`HaleHound-CYD by [JesseCHale]`、上游 GitHub 链接与 CiferTech 的引脚修正说明（`README.md:47/224/1342/1346`）。**但没有按 LICENSE 建议的 `"Based on HaleHound-CYD by Jesse C. Hale"` 形式显式声明「这是衍生作品 + 改了什么」**，也没有独立的 `THIRD_PARTY_NOTICES`。

### 4.2 上游 v4.0 的许可（换证后）

```
                     HALEHOUND-CYD PROPRIETARY LICENSE
Copyright (c) 2026 Jesse C. Hale. All rights reserved.

PERMITTED:
  - Flashing and running the firmware on compatible hardware you own
  - Using the firmware for authorized penetration testing ...
  - Sharing a link to this repository ...

PROHIBITED:
  - Redistributing the firmware binaries, in whole or in part, through
    any channel other than this repository OR the flash.HaleHound.com web flasher
  - Reverse engineering, decompiling, or disassembling the firmware
  - Modifying, adapting, or creating derivative works from the binaries
  - Selling, sublicensing, or commercially distributing the firmware
  - Hosting mirrors or alternative download locations for the binaries
  - Removing or altering any branding, attribution, or license notices
  - Claiming authorship of the firmware or any portion thereof
```

另含：`SOURCE CODE: ... is proprietary and not publicly available`；ATTRIBUTION NOTICE 保留 CiferTech 的 MIT（原 ESP32-DIV）；第三方清单（10 项）并声明 **"This project contains zero GPL-licensed code"**，其中两条自研替代被点名：`nrf24_raw.h`、`halehound_fft.h`（"written from public specifications"）；LGPL 组件给出 3 年供给源码声明。

**⚠️ 许可文本与其仓库内容不一致（S1 事实）**：许可反复以「the compiled firmware binaries in this repository」为对象，但该仓库**从未包含 `.bin`**（§2）。这不是小事 —— 它说明**这套条款实际上是用在 `flash.halehound.com` 的分发上**，而刷写器背后构建什么、由谁构建，外部无法审计。

### 4.3 对本 fork 的四个含义（需要 BOSS 拍板）

| # | 含义 | 依据 | 风险等级 |
|---|---|---|---|
| L1 | **MIT 授权对本 fork 已拿到的版本不可撤销**，本 fork 可以继续用 MIT 分发自己的代码；但**不能把 v4.0 的新材料当 MIT 用** | fork LICENSE = MIT；上游 v4.0 = 专有 | 低（现状合法） |
| L2 | **不能把上游 v4.0 的 README 文本 / 菜单文案 / branding 资产逐字搬进本 fork 的对外物料**（"Documentation and README files"、"ASCII art and branding assets" 都在专有 SCOPE 内） | 上游 LICENSE「SCOPE」「PROHIBITED」 | **中（当前文档里已有大量上游同名条目描述，需确认是「复述功能名」而非「抄文案」）** |
| L3 | **`HaleHound™` 是上游商标**，本 fork 的产品命名/包装/营销**不应使用该标记**（尤其 NM 系列商品化时） | 官网 `HaleHound™`；上游 LICENSE「Claiming authorship…」 | **高（涉及商品化）** |
| L4 | 本 fork 需补 **`THIRD_PARTY_NOTICES`** 与显式衍生声明；并按依赖实际许可处理 GPL（§5） | fork LICENSE「ATTRIBUTION REQUIREMENTS」「THIRD-PARTY」 | **高（决定能不能发二进制）** |

> 本节只陈述**从原文可直接读出的**内容与后果，不构成法律意见。L2/L3/L4 建议由 BOSS 决定后再落到具体动作（`TODO.md` Track A）。

---

## 5. 依赖许可证审计（S1，本地一手；复跑命令见 §11）

| 依赖（本 fork `platformio.ini`） | 包内证据 | 判定 | 风险 |
|---|---|---|---|
| `nrf24/RF24@1.4.11` | `library.json`: `"license": "GPL-2.0-only"`；`LICENSE`: "GNU GENERAL PUBLIC LICENSE / Version 2, June 1991" | **GPL-2.0-only** | 🔴 **与专有/闭源分发不可兼容；且与 GPL-3.0 不可组合** |
| `kosme/arduinoFFT@2.0.4` | `LICENSE` 正文："GNU GENERAL PUBLIC LICENSE / Version 3, 29 June 2007"；`library.json`/`library.properties` **无 license 字段** | **GPL-3.0**（按 LICENSE 正文） | 🔴 同上；与 RF24 的 GPL-2.0-only **互相冲突** |
| `sui77/rc-switch@2.6.4` | 包内**无 LICENSE 文件**（`license.txt` 亦无） | 未定；上游 README 称 LGPL-2.1 | 🟠 需补证 |
| `mikalhart/TinyGPSPlus@1.1.0` | 包内**无 LICENSE 文件** | 未定；上游 README 称 LGPL-2.1 | 🟠 需补证 |
| `lsatan/SmartRC-CC1101-Driver-Lib@2.5.7` | `LICENSE.txt`：**自定义授权文本**（"Mod by Little Satan. With permission to modify and publish Wilson Shen (ELECHOUSE)"），无 SPDX | 非标准 | 🟠 需判定 |
| `bodmer/TFT_eSPI@2.5.43` | 包内许可文件正文含 MIT 字样 | MIT（上游 README 列为 FreeBSD，**两处不一致**） | 🟡 人工确认 |
| `bblanchon/ArduinoJson@7.4.3` | `LICENSE`: The MIT License | MIT | 🟢 |
| `adafruit/Adafruit PN532@1.3.4` | `LICENSE`: BSD | BSD | 🟢 |
| `t-vk/ESP32 BLE Keyboard@0.3.2` | 包内**无 LICENSE 文件**（仅 `library.properties`，无 license 字段） | 未定 | 🟠 需补证 |
| 传递依赖 `Adafruit BusIO` | `LICENSE`: The MIT License | MIT | 🟢 |

**两条硬结论**

1. **GPL-2.0-only（RF24）与 GPL-3.0（arduinoFFT）不能共存于同一作品**，而本 fork 两者都链接进固件。也就是说：**即使抛开上游许可，本 fork 当前也无法合法分发编译产物**（无论标 MIT 还是专有）。
2. 上游为绕开同一问题，**专门自写了 `nrf24_raw.h` 与 `halehound_fft.h`**，并在 LICENSE 里把它当成卖点（"zero GPL-licensed code"）。⟹ 本 fork 要么做同样的替换，要么接受「整个固件按 GPL 分发」，二者必选其一（`TODO.md` A2）。

> **注**：「包内无 LICENSE 文件」不等于「无许可」，而是**本 fork 无法从自己手里的证据判定**。按其上游仓库标注（rc-switch / TinyGPSPlus = LGPL-2.1，t-vk BLE Keyboard = MIT）仍需一次人工核对并留档。

---

## 6. 社区施工手册（S2）

| 项 | 值 |
|---|---|
| 文件 | `HaleHound-CYD-Build-Guide-v141.pdf`（官网 CDN，9 153 471 B，**18 页**，文字层可提取） |
| 作者 | **Bkbroiler**（PDF 元数据 `/Author: Bkbroiler`；即上游 README 致谢名单里的 `BKBROILER`） |
| 性质 | **第三方撰写、官网在售的施工手册**（非上游官方文档） |

**内容结构（逐页）**：免责与允许用途 → 套件清单 → 硬件辨识 → 模块选购 → 供电 → JST 制作 → **ESP32 引脚图 + 全模块颜色对照表** → 外壳/套件链接 → 走线 → 组装 → 电池 → 收尾。

**套件清单（原样，可作本 fork 自己的 kit BOM 参考）**：E07 CC1101 · E01 NRF24 · 2 组 JST 线 · SPI 2.8/3.5 CYD · **PN532 RFID** · 5V USB-C 充电板 + 5V boost · 5V→3.3V buck · 洞洞板或 **Cinder Ferret** · 2× SMA 天线座 · 外壳与螺丝 · Ebyte 用 1.25 JST。
配套器件型号（手册「Device Documentation」表）：IP5306（boost/充电，Hale 套件）、TP4057（DIY 充电）、AMS1117（buck）、Mini PN532（可选）。

**该手册提供的、官方 README 没有的信息**：
- **按线色 + 逐模块的引脚对照表**（Brown=IO23 MOSI / Grey=IO19 MISO / Purple=IO18 SCK / Light Green=IO17 **PN532 SS** / Dark Blue=IO26 NRF CSN / Pink=IO16 NRF CE / White=IO21 CC1101 CS(3.5") / Light Blue=IO22 GDOTX & IO4 TX_EN / Dark Blue=IO35 GD1RX / Green=IO0 RX_EN / Red=供电 / Black=GND）。
- **OG CYD（非 SPI 版）的差异**：CC1101 CS=27、NRF CS=4、CC1101 TX=26、RX=0。
- 「**如果你 DIY，Hale 的外壳因空间限制不兼容你买到的便宜模块**」——平台壳体约束是真实的产品约束。

**该手册与官方 README 的冲突**：PN532 DIP 拨杆描述不同（官方 "CH1=OFF, CH2=ON"；手册「下拨右、上拨左」）——见 §3.4。

**该手册不覆盖**：固件行为、各模块操作流程（全在 README）、故障排查。⟹ 社区缺的是**「刷完之后怎么用」**这一类文档，本 fork 若做说明书，缺口就在那里。

---

## 7. 社区 issue 反馈（S1，按主题聚类）

上游 issue 是最硬的一手「使用评价」——不是评测站的形容词，而是有人在焊台上遇到的具体故障。以下为本次抓到的一手列表（截至 2026-10-16，近期条目）：

| 主题 | issue | 现象 | 本 fork 现状 |
|---|---|---|---|
| **共享 VSPI 下 SD 不识别** | **#165**（9 评论）/ **#168** 其重复单（7 评论） | 「E01、E07、PN532 都接上后 SD 卡检测不到」 | ❌ **无总线仲裁层**（§3.4）→ 新增 `H` 系列 |
| **CC1101 冻结** | **#172**（**14 评论**，最多） | CYD 上 CC1101 相关操作卡死 | 🟠 对应 `F1/F2/F3`（SubGHz 三件），无冻结对策 |
| SubGHz UI 冻结 / 频谱仪不工作 | #181（6）/ #180 | 进 SubGHz 后 UI 冻结；频谱仪不出图 | 🟠 `F1` |
| Tools / Skimmer 冻结且**无日志** | #183 | DIYmall 2432S028R + v3.9.5，PSRAM 初始化失败 + Tools/Skimmer 子菜单冻结、串口无输出 | 🟠 `F13` + 缺「无日志也要有屏上诚实上报」；本 fork 的 `A10` 空壳与之相关 |
| **Brownout 重启循环** | #182（3） | 空板开机就 brownout，屏不亮 | 🟠 `3.2` 的独立供电是根因方向；新增 `H` 系列巡检 |
| 16 MB flash 装不上 | #167（3） | 「Error installing on 16mb flash CYD」 | 🟠 本 fork 有 `PART_16MB=1`，需**闪存/分区矩阵**验证（`F` 系列） |
| **蓝牙子菜单没有返回主菜单** | **#187** | NM-RF-Hat + 2432S028 | ✅ **本 fork 已有** `Back to Main Menu`（`LegacyApp.inc:441`） |
| Loot 里删不掉凭据 | #184 | 无法删除已捕获凭据 | ❌ 新增（`D` 系列，对应 `F4`） |
| **Drone RID Spoof 崩溃** | **#189（未关闭）** | v4.0 下 RID Spoof 报错崩溃 | 🟠 `F10`；且这是 **v4.0 新功能当下就挂**，说明上游新模块成熟度低 |
| **可重指派射频引脚** | **#179**（6，功能请求） | 「希望引脚可配置，从而做成几乎免焊的 HaleHound/Cinder Ferret」 | ❌ 新增 `H3` —— 直接对应本 fork 的 `Pn532I2cDriver` 孤儿与接线分歧 |
| 别的板子/屏幕支持 | #170（3.2"）/ #169（**ST7789 驱动**）/ #185 | 用户想要更多板型 | 🟠 与 `§3.1` 同一件事 |
| **电池电量表** | #173 | 请求 fuel gauge | ❌ 新增 `D6`（本 fork 的 NM-EPD-420 产品需求里也有「电量仅屏幕显示」） |
| GPS 冷启动时长 | #162（**13 评论**） | 冷启动要多久、怎么提示 | ❌ 新增 `D5`（对应本 fork 未做的 GPS 屏） |
| 串口通信困惑 | #174（2） | 不知道串口怎么用 | 🟠 文档缺口（`G` 系列） |
| 拿不到 `.bin` | #186 | 「bin file download」 | ✅ 本 fork 自带构建与 `scripts/package_firmware.ps1` |
| Web Flasher 地区不可用 | #163 | 印度不可用 | ⚪ 与本 fork 无关（本 fork 不依赖它） |
| 24GHz Rd-01 雷达 | #188（已关闭） | 请求支持 | 🟠 对应 `F14`（需新硬件） |

**从这批 issue 能读出的三条规律**（对本 fork 的排期有直接影响）：

1. **上游最大的技术债是「共享总线 + 看门狗 + 无日志」这一类系统性问题**，不是功能缺失；`#165/#168/#172/#181/#183` 全属这一类，且评论数最多。
2. **上游新功能（v4.0 的 Drone RID Spoof）上线即挂**（#189 未关闭）——「功能清单长度」不等于成熟度，本 fork 不必追条目数。
3. **上游确实缺「用起来」的文档**：串口怎么用（#174）、GPS 要等多久（#162）、Loot 删不掉（#184）。本 fork 的差异化机会在这里。

---

## 8. 第三方评测（S3）与它们的错在哪

| 来源 | 性质 | 可用的东西 | **不可用的东西（已核错）** |
|---|---|---|---|
| `kunalganglani.com` / `bizarro.dev.to`（同作者两投） | SEO 评测（自称「两周实测」） | 公众认知锚点：**「$15 级 CYD = Pwnagotchi 替代品」**；成本分解（板 $7–15，四模块后 ≈$26–37 AliExpress / $55–71 Amazon）；与 Marauder 的对比框架 | 称最新为 **v3.7.2**、star **1400+**（实测 **v4.0 / 1812**）；把 HaleHound 说成「a platform… some are forks of Spacehuhn's deauther」——**与其自身描述矛盾** |
| `kaiyuanbang.cn`（自动生成撮合/选型页） | 机器生成摘要 | 两条**可核验**的观察：许可 `NOASSERTION`（✅ 与 API 一致）、「公开信息有限、缺乏完整官方文档」（✅ 与 §6 一致） | 版本 **v3.6.2**、star **1368**（均过时） |
| `zread.ai` 的 issues 汇总页 | **AI 生成的** issue 综述 | 线索价值：指出「源码关闭 vs 官网宣称 open source」的社区张力；XPT2046 引脚差异表 | **不能引用其结论**；它把不同年份、不同仓库的 issue 混编（例：把 CiferTech ESP32-DIV 的问题并入），且引用的 issue 编号/日期无法逐条对上 |
| `github.whanghao22.workers.dev`（第三方镜像的技术页） | 镜像 | **血统证据**：上游自述源自 `ESP32-DIV HaleHound v2.5.0`；旧版（v3.1.0）手册含引脚参考表 / SPI 共享 / Known Issues | 版本早已作废（v3.1.0） |

**许可争议的一手依据（S1，可复跑）**：上游仓库的 29 个 commit 里，`v2.6.0` 之后的提交**只动 `README.md` / `LICENSE`**，且 v2.6.0 的提交信息写着「Source code available by request」；官网同时写着「Community driven. No gatekeeping.」——**官网与仓库在「开源」这件事上不一致**。本 fork 的策略（按 release notes 独立实现）恰好绕开了这一点，但要**避免在自己的物料里复述「open source」而踩到同名争议**。

---

## 9. 外部证据 ⟹ 新增/升级的待办（映射表）

| 新 ID | 事项 | 依据 | 级别 |
|---|---|---|---|
| **C1** | 依赖许可合规：GPL-2.0-only（RF24）+ GPL-3.0（arduinoFFT）二选一处理 | §5（S1 本地） | **P0** |
| **C2** | `THIRD_PARTY_NOTICES` + 衍生声明（LICENSE 明列要求） | §4.1 | P0 |
| **C3** | 商标与命名边界（不使用 `HaleHound™`；产品化前定稿） | §3.7 / §4.3 L3 | **P0（若商品化）** |
| **C4** | 不搬运 v4.0 专有物料（README 文案/菜单文案/美术） | §4.3 L2 | P0 |
| **H1** | 共享 VSPI **仲裁层**（SD + CC1101 + NRF24 + PN532） | §7 #165/#168（S1） | **P0** |
| **H2** | PN532 **总线决策**：SPI（上游/社区标准）还是 I2C（本 fork 现状） | §1.3 / §3.4 | **P0** |
| **H3** | 射频引脚**可重指派**（配置化） | §7 #179（S1 功能请求） | P1 |
| **H4** | 闪存/分区矩阵（4 MB vs 16 MB，`PART_16MB`） | §7 #167 | P1 |
| **H5** | 独立供电/上电顺序巡检（brownout 对策） | §3.2 + §7 #182 | P1 |
| **D4** | Loot 条目**可删除** | §7 #184 | P1 |
| **D5** | GPS 屏：冷启动状态与等待提示 | §7 #162（13 评论） | P1 |
| **D6** | 电池电量显示 | §7 #173 | P2 |
| **D7** | 菜单分页/滚动（本 fork 蓝牙菜单 10→15 行导致行高 25→16 px） | `REMAINING_WORK §0.7` + §7 #187 | **P1** |
| **K1** | 固件/分区 `.bin` 一致性校验（`PART_16MB` 与 4 MB 变体都要能出包） | §7 #167 + 本 fork `T5` | P1 |
| **G1** | 「刷完之后怎么用」说明书（上游明确缺：串口 #174 / GPS #162 / Loot #184 / PN532 DIP 冲突 §3.4） | §6 / §7 | P1 |
| **G2** | 锁死 SD 路径契约（8 条已与上游一致）加回归测试 | §3.5 | P2 |

> **ID 约定**（本文与 `TODO.md` 共用）：`C` = 合规与许可、`H` = 硬件兼容与共享总线、`K` = 构建·分发·CI、`D` = UI/UX、`G` = 文档。
> 本文的 `D4–D7` 由外部证据直接登记（`D1–D3` 未分配），后续新增的 UI 项从 `D8` 起编。
> `U*` / `W*` / `R*` / `B*` / `F*` / `T*` / `X*` 系列**沿用 `REMAINING_WORK.md` 的既有编号，不重编**。

> 表中 `D`/`F`/`G` 编号与 `TODO.md` 的 Track 对应；`C` = 合规、`H` = 硬件兼容。

---

## 10. 未取得 / 无法验证（诚实清单）

| 项 | 原因 |
|---|---|
| 上游**源码** | 不存在（专有、不可得）→ 本 fork 永远只能「按文档独立实现」，无法逐行比对 |
| 上游**固件二进制** | release 零固件资产；只经 `flash.halehound.com` 分发，**外部无法审计其构建内容** |
| web flasher 背后的构建流程 | 同上 |
| 官方 Discord | 未定位到公开邀请链接（评测文曾提及「Discord channels」，未证实） |
| YouTube 逐条内容 | 只拿到频道（`@HaleHound07`）与作者演示视频链接；**未逐条观看**，故不用其内容作依据 |
| Build Guide 的**图片**内容 | PDF 图片层未解析（只提取了文字层）；接线图细节以文字表为准 |
| `0xFFF0` / `0xFD40` 等协议映射 | 仍归各 `docs/protocol-evidence/*.md`，本次未动 |
| 第三方评测的「实测」 | 无法核验（无原始数据、无照片、版本对不上）；已降级为 S3 |

**文档卫生问题（顺手发现，与本次内容无关）**：`docs/upstream-analysis/_gap_a.md` / `_gap_b.md` 这两个文件名暗示「差距分析」，实际是 v3.5.0–v3.9.x 的 **release 原文抓取**；真正的差距分级只在 `RELEASE_BRIEF_zh.md §5`。容易误读，建议改名（见 `TODO.md` G3）。

---

## 11. 复现命令

```bash
# 1) 上游 README（说明书全文）与 LICENSE（专有文本）
curl -sSL -o hh_readme.md  https://raw.githubusercontent.com/JesseCHale/HaleHound-CYD/main/README.md
curl -sSL -o LICENSE.txt   https://raw.githubusercontent.com/JesseCHale/HaleHound-CYD/main/LICENSE

# 2) 仓库元数据 / 目录 / release（注意：未认证 API 对 issues 端点返回 401）
curl -sSL -H "Accept: application/vnd.github+json" \
  https://api.github.com/repos/JesseCHale/HaleHound-CYD -o repo.json
curl -sSL -H "Accept: application/vnd.github+json" \
  "https://api.github.com/repos/JesseCHale/HaleHound-CYD/releases?per_page=100" -o releases.json
python -c "import json;d=json.load(open('releases.json'));print(len(d),sum(len(r['assets']) for r in d),[r['tag_name'] for r in d if r['assets']])"

# 3) 社区施工手册（18 页，需 pypdf 提文字层）
curl -sSL -o hh_build_guide_v141.pdf \
  "https://halehound.com/cdn/shop/t/4/assets/HaleHound-CYD-Build-Guide-v141.pdf?v=157209395051550384641781748372"
build-reports/hostenv/Scripts/python.exe -m pip install -q pypdf
build-reports/hostenv/Scripts/python.exe -c "from pypdf import PdfReader;r=PdfReader('hh_build_guide_v141.pdf');print(len(r.pages));print('\n'.join((p.extract_text() or '') for p in r.pages[7:9]))"

# 4) 本地依赖许可审计（在仓库根跑）
for d in .pio/libdeps/esp32-cyd/*/; do n=$(basename "$d"); \
  echo "== $n"; grep -m1 -i "license" "$d/library.json" 2>/dev/null; \
  for f in "$d"LICENSE* "$d"COPYING* "$d"license.txt; do [ -f "$f" ] && echo "   $(basename "$f"): $(grep -m1 -i 'GNU\|MIT\|BSD\|Apache\|Copyright' "$f")"; done; done
```
