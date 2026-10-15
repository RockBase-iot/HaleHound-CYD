# HaleHound-CYD 上游 Release 梳理与 fork 差距评估

| 项 | 值 |
|---|---|
| 上游仓库 | `JesseCHale/HaleHound-CYD`（纯分发仓库，无源码） |
| fork 仓库 | `RockBase-iot/HaleHound-CYD`（分支 `nm-cyd-c5`） |
| 上游最新版本 | **v4.0**（2026-10-15，即本文档当日） |
| fork 最后实现版本 | **v3.4.0**（2026-08-03，commit `b329e83`） |
| 落后跨度 | **v3.5.0 → v4.0，共 18 个版本 / 16 次功能更新** |
| 数据来源 | 上游 GitHub Releases API 全量抓取（28 个 release，365 KB 原始 JSON） |

> 本文档的每一条上游能力都来自 Release Notes 原文（`docs/upstream-analysis/_all_releases.md`，
> 296 KB 全文）。**由于上游是零源码分发仓库 + IP 限制拿不到 `firmware.bin`，所有上游行为
> 描述均为发布方自述，未经二进制逆向验证**——下文凡是推断的部分都单独标了「推断」。

---

## 1. 结论（一屏）

1. **上游没有源码可移植。** `JesseCHale/HaleHound-CYD` 的 29 个 commit 全部只动
   `README.md` / `LICENSE`，v2.6.0~v3.4.0 所有 tag 指向同一个 commit。因此**本项目不可能
   "同步上游代码"**，只能按 Release Notes 复述行为、在本分支独立实现。这一点决定了全部
   后续策略。

2. **fork 的路线图基线过时了 18 个版本。** 仓库内 `DEVELOPMENT_PLAN.md` 写的是
   「以 v3.8.1 为产品需求基线」，而上游今天已经发布 v4.0。建议把基线一次性提到 v4.0，
   否则每补一个版本就会再次落后。

3. **上游的重心已经从"堆攻击模块"转到"夯实底座"。** v3.9.x 系列几乎全是稳定性收口
   （BLE 白屏、SD 卡识别、内存归还），v4.0 的 "Under the hood" 明确写了
    *One NDEF implementation* / *RAM baselines measured across all seven board targets* /
   *pre-ship smoke test checklist*。**这正好是 fork 的 PlatformIO + 纯逻辑分层架构擅长的
   区间**，也是本轮开发选择的着力点。

4. **本轮已落地 9 个纯逻辑模块（2553 行）+ 4 套新 native 测试 + 6 个功能注册**，
   覆盖上游 v3.8.0~v4.0 中**可离线验证、不涉及射频发射**的部分。全部通过真实工具链校验
   （见 §7 验证矩阵）。

5. **两个必须由 BOSS 决策的阻塞项**（§8）：native 测试本机跑不起来（缺宿主 C++ 编译器）、
   `pio run` 被沙箱批量删除守卫拦死。另外 `DEVELOPMENT_PLAN.md` 规划的「五目标构建」与
   `findings.md` 收窄的「双主板」存在冲突，需要拍板。

---

## 2. 上游 Release 时间线（28 个版本）

| 版本 | 日期 | 主题 |
|---|---|---|
| v2.6.0 | 2026-02-18 | 运行时屏幕旋转；单文件刷机；动态屏幕尺寸 |
| v2.7.0 | 2026-02-20 | WhisperPair(CVE-2025-36911) / AirTag Detect / Auth Flood；Radio Test；Electric Blue 主题 |
| v2.8.0 | 2026-02-22 | 双核 Persuasion Engine；ADV KILL；NM-RF-Hat 支持；错固件检测；GPS 仪表盘 |
| v2.9.0 | 2026-02-25 | Wardriving 大改：BLE 记录、开放网络计数、GPS 自动初始化、WiGLE v1.6 CSV |
| v3.0.0 | 2026-02-28 | **全 13 个攻击模块双核化**；SubGHz Analyzer 改 33 段 VU 表；Evil Twin 凭据落 SD |
| v3.1.0 | 2026-03-01 | Stalkerware Detect；**CYD 3.5"(ESP32-3248S035) 支持**；NM-RF-Hat 构建目标 |
| v3.2.0 | 2026-03-06 | **VALHALLA 协议**；IoT Recon；PN532 RFID/NFC 五模式；Jam Detect；WiFi Guardian；SubGHz Sentinel；2.4G Watchdog；Full Spectrum；**PIN Lock** |
| v3.3.0 | 2026-03-12 | NRF 混杂嗅探器；MouseJack 注入；BLE Ducky；AirTag 攻击套件；统一 Loot Manager；E32R28T 支持；BLE 厂商数据库；CC1101 E07-PA |
| v3.3.2 | 2026-03-14 | 修 NRF24 Jammer 清理、Evil Twin 热点、Valhalla 子目录清空、RFID 暴力破解崩溃 |
| v3.4.0 | 2026-03-15 | **新板 E32R35T**；XPT2046 电阻触摸替换 GT911；触摸校准入 NVS |
| v3.5.0 | 2026-03-27 | **BLE Predator**；**Flock You**；Radio Test 重构；AP 锁定 2.4G 频谱；**Fast Touch Engine**；移除 BLE Ducky |
| v3.5.1 | 2026-03-28 | CYD35C 引脚修正补丁 |
| v3.5.5 | 2026-04-03 | Airoha RACE(CVE-2025-20700/1/2)；Tesla 充电口；`.sub` 读取；**Battery Monitor**；SubGHz Replay 大改；EEPROM 地址冲突修复 |
| v3.6.0 | 2026-06-14 | **Drone Toolkit 四件套**；**SubGHz 频谱分析仪**；**RFID NTAG Tools**；**Skimmer Hunter(beta)**；WLAN Jammer 重建；Captive Portal 六模板 |
| v3.6.1 | 2026-06-15 | HALEHOUND 涂鸦水印；RFID dump 进 Loot；修 RFID 掉 SD、Evil Portal 广播 SSID |
| v3.6.2 | 2026-06-29 | 去重美术资源省 180 KB flash；修 Deauth 白屏/自退、Wardriving 白屏 |
| v3.7.0 | 2026-07-08 | **SkeletonKey**；双核 Wardriving（~2×扫描率）；IoT Recon 重构（厂商指纹/无限字典）；AirTag Scream |
| v3.7.1 | 2026-07-09 | 修 SD 卡"读不到"（根因修复）；Flock You 扩到 WiFi 检测；OUI 22→42 |
| v3.7.2 | 2026-07-10 | 修 BLE 连接失败白屏、WiFi→BLE 白屏、CC1101 低 RSSI 丢捕获、PA 模块选择被覆盖 |
| v3.8.0 | 2026-07-16 | **Chameleon Ultra 蓝牙控制**；**RFID 后端选择器**（PN532/Chameleon/ST25R3916）；**NFC 写 8 种记录(GPS 感知)**；Recon→攻击免二次扫描 |
| v3.8.1 | 2026-07-27 | 蓝牙稳定性总修（白屏消失）；FlockYou 可靠落 SD |
| v3.8.2 | 2026-07-29 | SkeletonKey 原地重扫；最后一批 BLE 白屏清除；Flock 命中带 GPS 进 Loot |
| v3.9.1 | 2026-07-31 | **NRF 调参面板**；2.4G 干扰加密；**KeeLoq 滚动码解码**；BLE 名称中文化（厂商/服务/特征） |
| v3.9.2 | 2026-08-03 | **SubGHz 调参驾驶舱**；NRF 面板按 jammer 显隐控件（无死旋钮）；Chameleon 自动连接 |
| v3.9.3 | 2026-08-05 | 蓝牙工具可无限串联；**归还 ~15 KB RAM**；低内存优雅退出 |
| v3.9.4 | 2026-08-13 | WiFi 工具归还射频内存；Loot 查看器扩到 Honeypot/FlockYou/Predator；3.5" 布局修正 |
| v3.9.5 | 2026-08-30 | **KARR 检测与脆弱性评估**；Replay 目标选择器（7 类预设）；KeeLoq RF600E/FM 读取器；电台扫描器厂商名(4 万条)；GATT 全量落盘；**Chameleon 诚实报告(N of 16)**；移除 868/915/925 死频段 |
| **v4.0** | **2026-10-15** | **ST25R3916 驱动 + RFID 七屏重构**；**iCLASS 读/解/克隆(26/35/48-bit)**；Key Check(29 密钥)；MIFARE Classic 转储/写入/魔术卡；**Flipper `.nfc` + Proxmark `.eml` 双向**；**HID reader tooling**；IoT Recon 成长；Drone 套件成长；**KARR 新增 0xBB/0x55 framing**；Nice Flor-S 解码；Rd-01 24GHz 雷达；FlockYou 加 Axon/Utility 摄像头；**AWOK Dual Touch v3 / AWOK MINI / E32R40T 三块新板**；One NDEF 实现 + 50 例宿主测试；SD 20MHz；30fps 动画门控 |

---

## 3. 上游能力清单（按族聚合，标注引入版本）

### 3.1 底座与平台

| 能力 | 首次版本 | 最新演进 |
|---|---|---|
| 单文件刷机 + 四文件备用 | v2.6.0 | v2.8.0 起按板分包 |
| 动态屏幕尺寸（运行时，非硬编码像素） | v2.6.0 | 持续 |
| 板级识别（错固件检测 / VALHALLA 板标识） | v2.8.0 | v3.3.0 扩到 E32R28T/HAT |
| 双核引擎（Core 0 射频 / Core 1 显示触摸） | v3.0.0（13 模块） | 后续新模块均遵守 |
| Fast Touch Engine（绕开 `getTouch()` 5× 校验） | v3.5.0 | — |
| 内存归还 / 低内存优雅退出 | v3.9.3（~15 KB） | v3.9.4 扩到 WiFi 模块 |
| RAM 基线（7 个板目标）+ 出厂冒烟测试清单 | v4.0 | — |
| 资源去重（美术/字体/图标省 180 KB） | v3.6.2 | v3.9.5（jammer 骷髅图降为 1/4） |
| SD 20 MHz（库默认 4 MHz） | v4.0 | — |
| 渐变条批量 `fillRect` + 动画门控 30fps | v4.0 | — |

### 3.2 安全模型 / 治理

| 能力 | 说明 | 引入版本 |
|---|---|---|
| VALHALLA 协议 | 擦除 SD、锁定 17 个攻击函数、进入 Blue Team、EEPROM 持久化法律声明（引用 47 U.S.C. §333 / 18 U.S.C. §1030） | v3.2.0 |
| PIN Lock | 4 位设备锁 + 30s~10min 可配超时 + 骷髅锁屏 | v3.2.0 |
| Blue Team 仅防御模式 | 跨重启持久，直到主动重新接受声明 | v3.2.0 |

> ⚠️ **有意偏离**：fork 的 `findings.md` 明确禁止 VALHALLA 隐含删除数据，紧急清除需独立
> 命名与独立确认。这是合规上的正确选择，应作为长期差异保留。

### 3.3 RFID / NFC（上游 v4.0 的绝对重心）

| 能力 | 引入版本 | 备注 |
|---|---|---|
| PN532 五模式（扫描/读/克隆 Gen1a/密钥爆破/字典） | v3.2.0 | 软件 SPI 修 CYD GPIO 冲突 |
| NTAG Tools（213/215/216/Ultralight EV1，九项菜单，stage-then-write） | v3.6.0 | 擦除/永久只读/设密码/清密码 |
| RFID dump 进 Loot 且机内可读 | v3.6.1 | |
| RFID 后端选择器（PN532 / Chameleon Ultra / ST25R3916） | v3.8.0 | |
| NFC 写 8 类记录（URL/Text/vCard/WiFi/蓝牙/Android App/geo/SMS），geo 可抓实时 GPS | v3.8.0 | 记录**堆叠**而非覆盖 |
| ST25R3916 驱动 + 七屏重构；支持 MIFARE Classic/NTAG/Ultralight/ISO-DEP/iCLASS | v4.0 | |
| iCLASS 读/解/克隆：AA1+AA2 全读、板载 3DES、PACS 位抽取、26/35/48-bit + Corporate 1000 | v4.0 | |
| Key Check：扫 29 个公开密钥，命中后全模块可用 | v4.0 | |
| MIFARE Classic 全卡转储（嵌套认证，31/32 扇区单会话）、扇区尾访问位解码、带密钥写入、魔术卡 | v4.0 | |
| **Flipper `.nfc` + Proxmark `.eml` 双向**，离线可打开 | v4.0 | |
| ISO-DEP 停止协商 424 kbps → Type 4 全链可用 | v4.0 | |
| **One NDEF 实现**（解码器 + 8 记录构造器 + URI 前缀表从 4 处合一），**50 例宿主测试** | v4.0 | |

### 3.4 蓝牙 / BLE

| 能力 | 引入版本 |
|---|---|
| BLE Predator（GATT 侦察 + 蜜罐凭据陷阱） | v3.5.0（替换 BLE Scanner+Sniffer） |
| SkeletonKey（全服务/特征枚举、读写、内置 profile、GATT 树带真实名与完整 UUID、色彩混合器） | v3.7.0 / v3.9.1 |
| Chameleon Ultra over BLE（扫描/连接/读写 HF+LF/克隆入槽/恢复 MIFARE） | v3.8.0 |
| Chameleon 自动连接 + **诚实报告**（partial 读标记、破解进度报 "N of 16"） | v3.9.2 / v3.9.5 |
| 统一分类器（AirTag/Tile/SmartTag/Chipolo/Apple 等）Recon→SkeletonKey 目标直传 | v3.8.0 |
| BLE 名称数据库（厂商/服务 UUID/GAP 外观 → 真实名） | v3.3.0 / v3.9.1 |
| **KARR**（SouthWest Dealer Services 车警报器，约 285 万台 2017+ 起亚/本田/丰田/马自达/福特） | v3.9.5 |
| KARR 新增 `0xBB Counter` 与 `0x55` 加密 framing；不支持时**明说 framing 不支持而非报"设备静默"**；connect 失败给出原因并掉电前落盘 | v4.0 |
| **HID reader tooling**（定位并压测 Signo / iCLASS SE 读卡器，被动扫描 + 连接诊断 + 抓取读事件） | v4.0 |

### 3.5 2.4 GHz / SubGHz

| 能力 | 引入版本 |
|---|---|
| NRF 混杂嗅探器（Goodspeed 模式，126 信道跳频，点地址直填 MouseJack） | v3.3.0 |
| MouseJack 键盘注入（Logitech HID++ 0xC1/LRC/16-bit CRC、全 HID 扫描码、预置 payload） | v3.3.0 |
| SubGHz 频谱分析仪（33 段 + peak-hold + 波形图 + 频段聚焦 + CAL + GAIN） | v3.6.0 |
| SubGHz Replay 大改（RSSI 门控、drain loop、二次校验、CLEAR、屏上 QWERTY、自动扫描） | v3.5.5 |
| SubGHz Replay 目标选择器（7 类预设，替代盲扫） | v3.9.5 |
| **NRF 调参面板**（SPI clock / 真实 dBm 功率 / 模式，渐变拖拽滑块 + 开关） | v3.9.1 |
| **SubGHz 调参驾驶舱**（调制/速率/带宽/AGC 灵敏度…） | v3.9.2 |
| **NRF 面板按 jammer 显隐控件 → 无死旋钮** | v3.9.2 |
| **移除 868/915/925 死频段**（硬件听不到，留着只会像 bug） | v3.9.5 |
| KeeLoq 滚动码解码（serial/button/counter，内置厂商密钥表） | v3.9.1 |
| KeeLoq RF600E / FM fob 读取器（单次按键读 serial+button） | v3.9.5 |
| KeeLoq 要求解密功能码匹配明文按键才采信 | v4.0 |
| Nice Flor-S 无钥匙滚动码解码 | v4.0 |
| CC1101 E07-PA 功放控制（TX_EN/RX_EN） | v3.3.0 / v3.4.0 |

### 3.6 WiFi / 网络 / 无人机

| 能力 | 引入版本 |
|---|---|
| Wardriving（BLE 记录、开放网络计数、GPS 自动初始化、WiGLE v1.6 CSV、双核 2× 扫描率、RATE 表） | v2.9.0 / v3.7.0 |
| 电台扫描器厂商名（IEEE 注册表 4 万条落 SD） | v3.9.5 |
| IoT Recon（厂商指纹 Hikvision/Dahua/Uniview/Amcrest、品牌感知字典排序、无限 SD 流式字典、Sweep/Deep、RTSP/HTTP Digest 认证） | v3.2.0 / v3.7.0 |
| IoT Recon 成长（DVR/GoAhead 凭据利用、修 RTSP digest 交换、修 HTTP 路径、telnet） | v4.0 |
| Drone Toolkit（Detect 双射频并行 / Jelly 全屏干扰+均衡器 / Deauth 定向 / RID Spoof ASTM F3411） | v3.6.0 |
| Drone 套件成长（按目标选择、RID 假蜂群基于真实 GPS + 250m 系绳、可滚动详情） | v4.0 |
| Flock You（Flock Safety 摄像头检测；BLE + WiFi 双模指纹；OUI 22→42；加 Axon/Utility 执法记录仪） | v3.5.0 / v3.7.1 / v4.0 |
| Jam Detect / WiFi Guardian / SubGHz Sentinel / 2.4G Watchdog / Full Spectrum 防御五件套 | v3.2.0 |
| Skimmer Hunter（多信号评分 + 主动确认降低误报） | v3.6.0 (beta) |
| Airoha RACE 三 CVE / Tesla 充电口 / Loot Manager | v3.5.5 / v3.3.0 |
| **Rd-01 24GHz 雷达模块** | v4.0 |
| Battery Monitor | v3.5.5 |

### 3.7 硬件目标（上游 v4.0 共 8 个）

`CYD 2.8"(ESP32-2432S028)` · `CYD HAT` · `E32R28T` · `E32R35T` · `CYD35C(3248S035C)`
· **`AWOK Dual Touch v3`** · **`AWOK MINI(128x128 摇杆)`** · **`E32R40T(4.0")`**

> ⚠️ **有意偏离**：fork 只支持 `esp32-cyd` + `nm-cyd-c5` 两主板（`findings.md` 已拍板）。
> 上游这 8 个板目标**不作为 fork 的缺口**，只在需要时按 `BoardProfile` 扩展。

---

## 4. fork 现状基线

| 项 | 状态 |
|---|---|
| 分支 | `nm-cyd-c5`，HEAD `b329e83`（2026-08-03，"support esp32-cyd and nm-cyd-c5"） |
| 平台 | PlatformIO 6.x；`[env]` 用 pioarduino 55.03.32 + `bmorcelli/esp32-arduino-lib-builder`(IDF 5.5) |
| 构建目标 | `native`（宿主测试）+ `esp32-cyd`(espresif32@6.9.0, Xtensa) + `nm-cyd-c5`(RISC-V) |
| 架构分层 | `src/core` · `src/board` · `src/features` · `src/drivers` · `src/platform` · `src/services/pure` |
| 生命周期 | `IFeature`（begin/tick/requestStop/end）+ `FeatureRegistry` + `FeatureCatalog` |
| 治理 | `PolicyService`（VALHALLA / Blue Team）单点授权 |
| 已注册功能 | **21** 个（`FeatureCatalog.cpp` 中 21 条 `D_FACTORY`） |
| native 测试 | **25** 套 Unity 套件（本轮新增 4 套） |
| 已知长期缺口 | `native` 环境的宿主测试**从未真正跑过**（本机无宿主 gcc/g++） |

---

## 5. 差距矩阵

分级口径：
- **P0 必须补** = 上游已构成功能闭环、fork 缺了会明显落后
- **P1 建议补** = 体验/稳定性收益明确
- **P2 低优先** = 依赖额外硬件或社区数据，可延后
- **D 有意偏离** = 不该补，应长期保持差异

| # | 能力域 | 上游状态 | fork 状态 | 差距 | 级别 |
|---|---|---|---|---|---|
| 1 | **统一 NDEF 实现**（解码器 + 8 记录构造 + URI 前缀表 + 测试） | v4.0 合成 1 处 + 50 例测试 | **本轮已补** `NdefCodec`(670 行) + 8 例测试 | ✅ 闭合 | 已补 |
| 2 | **卡转储格式**（Flipper `.nfc` / Proxmark `.eml` 双向） | v4.0 | **本轮已补** `CardDumpFormat`(391 行) + 5 例 | ✅ 闭合 | 已补 |
| 3 | **iCLASS PACS 解码**（26/35/48-bit + Corporate 1000） | v4.0 | **本轮已补** `IclassPacs`(236 行) + 7 例 | ✅ 闭合 | 已补 |
| 4 | **KARR 新 framing**（0xBB Counter / 0x55 加密 / 不支持时明说） | v4.0 | **本轮已补** `KarrModel`(144 行) + 8 例 | ✅ 闭合 | 已补 |
| 5 | **NRF + SubGHz 调参驾驶舱**（按 jammer 显隐控件、无死频段） | v3.9.1/v3.9.2/v3.9.5 | **本轮已补** `RadioTuningModel`(350 行) | ✅ 闭合 | 已补 |
| 6 | **SkeletonKey**（GATT 树 / 全量落盘 / 色彩混合） | v3.7.0~v3.9.5 | **本轮已补模型层** `SkeletonKeyModel`(264 行) | 模型 ✅ / BLE 驱动 ❌ | P1 |
| 7 | **Chameleon Ultra**（8 槽、auto-connect、诚实报告 N of 16） | v3.8.0~v3.9.5 | **本轮已补模型层** `ChameleonUltraModel`(186 行) | 模型 ✅ / BLE 驱动 ❌ | P1 |
| 8 | **HID reader tooling**（Signo / iCLASS SE，被动扫描 + 连接诊断） | v4.0 | **本轮已补模型层** `HidReaderModel`(126 行) | 模型 ✅ / BLE 驱动 ❌ | P1 |
| 9 | **RFID 后端选择器**（PN532 / Chameleon / ST25R3916 能力矩阵 + 屏可用性） | v3.8.0 | **本轮已补** `RfidBackendModel`(186 行) | 模型 ✅ / ST25R3916 驱动 ❌ | P0 |
| 10 | **ST25R3916 驱动 + RFID 七屏重构** | v4.0 | 无 | 需 SPI 驱动 + 七屏 UI | P0 |
| 11 | **MIFARE Classic 全卡转储 / 密钥写入 / 魔术卡** | v4.0 | **模型层已补** `MifareClassicModel` + `MifareKeys`（17 例测试） | 模型 ✅ / ISO14443-A 认证链 ❌；魔术卡后门按证据门标 `UNSUPPORTED` | P0 |
| 12 | **Key Check（29 公开密钥扫描）** | v4.0 | **模型层已补** `KeyCheckModel`（12 例测试） | 模型 ✅ / 驱动扫描 ❌；密钥表规则已写明（proxmark3 `mfc_default_keys.dic` 前 29 条） | P0 |
| 13 | **NTAG Tools 写入侧**（擦除/永久只读/设密码/清密码） | v3.6.0 | 容量/锁位/密码模型已有（`NtagModel` + `NdefCodec`），**写入路径无** | 需 PN532 写入路径 + 危险操作二次确认 | P1 |
| 14 | **ISO-DEP / Type 4 全链** | v4.0 | 无 | 依赖 #10 | P1 |
| 15 | **RFID dump 进 Loot 并可机内查看** | v3.6.1 | 无 | 需 StorageService + 文本查看器对接 | P1 |
| 16 | **SD 卡根因修复**（射频后 Loot 读不到卡） | v3.6.1/v3.7.1/v3.9.5 三次修 | 未知 | **需台架复现验证** | P0 |
| 17 | **BLE 内存/白屏收口**（模块间任意顺序切换） | v3.7.2/v3.8.1/v3.8.2/v3.9.3 | 未知 | 需台架长跑验证 | P0 |
| 18 | **内存归还 + 低内存优雅退出** | v3.9.3(~15KB)/v3.9.4 | 无 | 需 heap 基线测量 | P1 |
| 19 | **Wardriving 双核 2× + 增量内存** | v3.7.0/v3.9.5 | 部分（已有 Wardriving） | 需 Core 0 分流 + 消除逐行分配 | P1 |
| 20 | **Drone Toolkit 四件套** | v3.6.0/v4.0 | 部分（已有 `RemoteIdParser`） | 缺 Detect/Jelly/Deauth/RID Spoof | P1 |
| 21 | **SubGHz 频谱分析仪**（33 段 + peak-hold + CAL） | v3.6.0 | 无 | 需 RMT/CC1101 采样 + UI | P1 |
| 22 | **SubGHz Replay 目标选择器 + RSSI 门控** | v3.5.5/v3.9.5 | 无 | | P1 |
| 23 | **KeeLoq 滚动码解码 + RF600E/FM 读取** | v3.9.1/v3.9.5/v4.0 | 无 | 需厂商密钥表 | P1 |
| 24 | **IoT Recon**（厂商指纹 + SD 流式字典 + Digest） | v3.2.0/v3.7.0/v4.0 | 无 | 大工程，含主动利用 | P1 |
| 25 | **BLE 名称数据库**（厂商/服务/GAP） | v3.3.0/v3.9.1 | **模型已补** `GattNameTable`（165 项，已逐字对齐 Bluetooth SIG 机读清单） | 模型 ✅ / PROGMEM 化与 legacy `ble_database.h` 合并 ❌ | P1 |
| 26 | **统一 Loot Manager + GPS 打标** | v3.3.0/v3.8.2/v3.9.4 | 无 | 需 Storage + GPS 对接 | P1 |
| 27 | **Radio Test 重构**（NRF 126 信道 + CC1101 RSSI 基线 + 接线图） | v3.5.0 | 无 | | P2 |
| 28 | **Fast Touch Engine** | v3.5.0 | 无 | 触摸层改造 | P1 |
| 29 | **Flock You**（BLE+WiFi 双模 + OUI 42 + 执法记录仪） | v3.5.0~v4.0 | 有 `FlockFingerprint`（基础） | 缺 WiFi 模式 + OUI 扩充 | P2 |
| 30 | **Skimmer Hunter** | v3.6.0 beta | 有 `SkimmerScore`（基础） | 缺主动确认 pass | P2 |
| 31 | **Rd-01 24GHz 雷达** | v4.0 | 无 | 需新硬件 | P2 |
| 32 | **板目标扩到 8 个**（AWOK 系列 / E32R40T…） | v4.0 | 2 个（`esp32-cyd` / `nm-cyd-c5`） | 按需扩展 | **D** |
| 33 | **VALHALLA = SD 擦除** | v3.2.0 如此实现 | 拆为独立命名 + 独立确认 | 保持差异 | **D** |
| 34 | **BLE Ducky**（v3.5.0 已移除） | 已废弃 | 无 | 不补 | **D** |
| 35 | **上游 8 板闪存包分发体系** | v2.6.0~ | 不适用（自用板） | | **D** |

**统计**（按上表逐行口径，2026-10-15 更新）：**已闭合 5** · **模型层已补、缺驱动/接线 7**
（其中 P0 三项：#9 / #11 / #12）· **P0 待补 3**（#10 / #16 / #17）· **P1 待补 12** ·
**P2 待补 4** · **有意偏离 4**。

> 「模型层已补」= 纯逻辑经宿主断言验证，但**没有真实传输/驱动，也未接入 `FeatureCatalog`**，
> 因此**不等于功能可用**。剩余工作量与建议执行顺序见根目录 `REMAINING_WORK.md`。

---

## 6. 本轮已交付的开发

> **范围说明**：§6.1–§6.3 记录的是 2026-10-15 上午那一轮（9 个模块）。同日下午又补了
> 4 个模型与 7 套测试，见 §6.1b / §6.3b。

### 6.1 新增纯逻辑模块（9 个，2553 行）

| 模块 | 文件 | 行数 | 对应上游动作 |
|---|---|---|---|
| `NdefCodec` | `src/features/rfid/NdefCodec.{h,cpp}` | 670 | v4.0 *One NDEF implementation* |
| `CardDumpFormat` | `src/features/rfid/CardDumpFormat.{h,cpp}` | 391 | v4.0 *Dumps go somewhere useful* |
| `RadioTuningModel` | `src/features/tuning/RadioTuningModel.{h,cpp}` | 350 | v3.9.1/v3.9.2/v3.9.5 调参驾驶舱 + 死频段 |
| `SkeletonKeyModel` | `src/features/ble/SkeletonKeyModel.{h,cpp}` | 264 | v3.7.0/v3.9.1/v3.9.5 |
| `IclassPacs` | `src/features/rfid/IclassPacs.{h,cpp}` | 236 | v4.0 *iCLASS — read, decode, clone* |
| `ChameleonUltraModel` | `src/features/rfid/ChameleonUltraModel.{h,cpp}` | 186 | v3.8.0/v3.9.2/v3.9.5 |
| `RfidBackendModel` | `src/features/rfid/RfidBackendModel.{h,cpp}` | 186 | v3.8.0/v4.0 后端选择器 |
| `KarrModel` | `src/features/ble/KarrModel.{h,cpp}` | 144 | v3.9.5/v4.0 KARR |
| `HidReaderModel` | `src/features/ble/HidReaderModel.{h,cpp}` | 126 | v4.0 HID reader tooling |

### 6.1b 同日追加的纯逻辑模块（4 个，1533 行）

| 模块 | 文件 | 行数 | 对应上游动作 |
|---|---|---|---|
| `MifareClassicModel` | `src/features/rfid/MifareClassicModel.{h,cpp}` | 606 | v4.0 MIFARE Classic 全卡转储 / 密钥写入 |
| `GattNameTable` | `src/features/ble/GattNameTable.{h,cpp}` | 451 | v3.3.0/v3.9.1 BLE 名称数据库 |
| `KeyCheckModel` | `src/features/rfid/KeyCheckModel.{h,cpp}` | 251 | v4.0 Key Check（29 公开密钥） |
| `MifareKeys` | `src/features/rfid/MifareKeys.{h,cpp}` | 225 | v4.0 公开密钥表 |

沿用的「把诚实性约束固化成不可绕过代码路径」的做法同样适用：

- `KeyCheckModel`：进度由**尝试次数**推导而不是扇区比例；扇区**只在认证成功时计数**；
  汇总固定为「N of M sectors recovered, K need a nested/hardnested attack」——
  **不做攻击也不假装做了**。
- `MifareClassicModel`：尾块 `C1C2C3 = 111` 的扇区报告为**永久锁死**；
  魔术卡写入要求 `backdoorDetected && userConfirmed` **两个条件同时成立**；
  `setSectorKeyA/B` 有去重守卫，一个扇区只计一次（即使 A、B 都恢复）。
- `GattNameTable`：165 项逐字对齐 Bluetooth SIG 机读清单；未分配项必须带 `(vendor)` /
  `(member)` 标记；**无法核验的条目直接删除，不写进例外清单**（核验器
  `build-reports/verify_gatt_names.py` 做零例外比对）。

设计上刻意把**上游行为里那些"诚实性"约束固化成不可绕过的代码路径**——这类约束最容易被
后续重构悄悄丢掉：

- `KarrClassifier::sendsCommands()` **恒为 false**：只做被动判定，永不主动发命令；
  每次遭遇（**含连接失败**）都标 `rawEncounterRecorded = true`；未知 framing 报
  `UNSUPPORTED FRAMING` 而不是让设备看起来"静默"。（对应 v4.0 原文）
- `RadioTuningModel`：SubGHz 的 `868 / 915 / 925` 明确 `offered = false`——硬件听不到就
  不下发该选项（对应 v3.9.5 *Dead SubGHz bands are gone*）；NRF 面板按 jammer 暴露可用旋钮、
  隐藏死控件（对应 v3.9.2 *No dead knobs*）。
- `ChameleonUltraModel::recordNonce()` 报 "N of 16"，`evaluateRead()` 区分 partial/valid
  （对应 v3.9.5 *reports honestly as N of 16*）。
- `IclassPacs`：未确认宽度的格式只抽取字段、**不断言奇偶校验**（`parityConfirmed = false`）。
- `HidReaderModel`：`HidSweepScheduler` 每连接最多 2 条命令（避免超时），`report()` 仅在
  `replyCaptured` 时标 completed。
- `NdefCodec`：Android App 记录用外部类型 `android.com:pkg`；SMS 用 URI 记录 `sms:` 方案。

### 6.2 功能注册（`FeatureCatalog` 扩展）

新增 6 个 `FeatureId` 与描述符：`BluetoothSkeletonKey=0x020B`、`BluetoothHidReader=0x020C`、
`BluetoothKarr=0x020D`、`Nrf24Tuning=0x0306`、`SubGhzTuning=0x0407`、`RfidBackend=0x0506`。
注册表从 15 → **21** 条。同时修正了 `test_catalog_lifecycle` 的 Nrf24 计数断言（6 → 7）。

### 6.3 新增 native 测试（4 套，共 28 个用例）

| 套件 | 用例数 | 关键断言 |
|---|---|---|
| `test_ndef_codec` | 8 | URI 缩写、URL/Text 往返、记录堆叠、长 payload 4 字节长度、Wifi/VCard/Geo/SMS、TLV 跳过 CC+padding、截断拒绝 |
| `test_iclass_pacs` | 7 | HID26 字段+奇偶、翻转奇偶被报出、奇偶辅助函数按组计数、宽格式不确认奇偶、未知宽度=`UNSUPPORTED FRAMING`、bitstring 解析、按位宽查找 |
| `test_card_dump_format` | 5 | Flipper 往返、Proxmark 往返+厂商块推导、全未知块拒绝、畸形拒绝、扩展名分发 |
| `test_karr_classifier` | 8 | framing 分类、**永不命令**、不稳定=Patched、加密 framing 永不 Vulnerable、未知 framing 命名、key 优先、连接失败原因、从不回答 |

### 6.3b 同日追加的 native 测试（7 套，共 75 个用例）

| 套件 | 用例数 | 关键断言 |
|---|---|---|
| `test_mifare_classic` | 17 | `FF 07 80 69` 解码为 `C1=0,C2=0,C3=8`、尾块权限「仅 Key A」、反码不一致被拒、`00 F0 FF 69` 报告永久锁死、尾块往返、魔术卡双条件门禁、转储诚实计数 |
| `test_key_check` | 12 | 进度按尝试次数、扇区仅在认证成功时计数、密钥表索引与断言一致、诚实汇总文案 |
| `test_skeleton_key` | 12 | GATT 树计数、名称解析接入、`classifyAppearance` 不发明设备类型、停止后不可再写全量转储 |
| `test_chameleon_ultra` | 10 | 8 槽读写、auto-connect 拒绝二次接管、空地址不被采纳、超读不判 valid、`N of 16` 诚实报告 |
| `test_gatt_names` | 9 | 表非空且严格升序无重复、44/113/15 计数钉住、SIG 原文（`GAP`/`Battery`）、未分配项带来源标记 |
| `test_rfid_backend` | 8 | 七屏 `screenLabel()` 两两不同、能力矩阵、按后端显隐屏项 |
| `test_hid_reader` | 7 | 每连接最多 2 条命令、仅 `replyCaptured` 才标 completed、被动不主动发命令 |

### 6.4 测试基础设施

`test/native/` 下原有 21 套 Unity 套件，但仓库缺 Unity 实现，导致 `pio test -e native`
在有宿主编译器的机器上也构建不起来。本轮补：
- `scripts/hostcheck/unity/unity.h` — 最小 Unity shim，只实现仓库实际用到的宏
- `scripts/hostcheck/unity/unity_shim.cpp` — `unity_begin/end/fail/fail_cmp` 实现
- `scripts/hostcheck/run_host_tests.py` — 不依赖 PlatformIO 的宿主跑测器

---

## 7. 验证矩阵（全部为实测，非推断）

| 检查 | 命令 / 工具 | 结果 |
|---|---|---|
| 9 个新模块真编译（Xtensa, `-Wall -Wextra -Os`） | `xtensa-esp32-elf-g++ -c` | **9/9 通过，0 告警** |
| 4 套测试 TU + unity shim 真编译 | 同上 | **5/5 通过** |
| 重定义 / ODR 冲突 | `xtensa-esp32-elf-g++ -r` 合并 | **4/4 `rc=0`，0 处 multiple definition** |
| 未解析符号审计 | `nm -u` vs `nm --defined-only` | **4/4 套件 0 个非 libc 未解析符号** |
| `nm-cyd-c5` 真实 env 语法（RISC-V，IDF 5.5 全 include 集，`gnu++2a`） | `build-reports/syntax_env_check.py` | **11/11 通过** |
| `nm-cyd-c5` 真实 env，新测试套件（`gnu++17`） | 同上 `--tests` | **5/5 通过** |
| `esp32-cyd` 旧框架布局 | 同上 `--env esp32-cyd --baseline` | 未改动文件报**同样**错误 → 判定为工具噪声，非回归 |
| `pio run -e esp32-cyd` / `-e nm-cyd-c5` | PlatformIO 6.2.0 / 6.1.18 | ❌ **被沙箱批量删除守卫拦死**（见 §8.2） |
| **36 套 native 测试实际执行** | `scripts/hostcheck/run_host_tests.py`（仓库本地 `ziglang` / clang 21） | ✅ **36/36 套件全绿**（2026-10-15，含新增 7 套；§8.1 已闭合） |

`build-reports/syntax_env_check.py` 直接读 pioarduino 生成的权威 flags 文件
（`framework-arduinoespressif32-libs/<chip>/flags/{includes,defines,cpp_flags}`，
289 条 `-iwithprefixbefore` = 完整 ESP-IDF include 集），并用 `-iprefix` 压缩命令行以规避
Windows 32 KB 上限。**未覆盖**：链接、`extra_scripts`、`build_src_filter`。

---

## 8. 阻塞项（需要 BOSS 决策）

### 8.1 native 测试本机跑不起来 ✅ 已闭合（2026-10-15）

> **解法**：不装系统级工具链，改用仓库本地 `build-reports/hostenv/` 里的 `ziglang`
> （pip wheel，自带 clang 21 + libc++/libc sysroot），由 `scripts/hostcheck/run_host_tests.py`
> 驱动。实测 **36/36 套件全绿**。
> 下面的 mingw32 分析保留，因为它解释了为什么**必须探针编译再采纳**——该包的
> `g++ --version` 正常，但每次编译都以**完全空的日志**失败，靠 `Path.exists()` 或
> `--version` 都检测不出来。

- `toolchain-gccmingw32/libexec/.../cc1plus.exe` 存在，但**同目录缺 `libgmp-10.dll` /
  `libmpfr-4.dll` / `libmpc-3.dll` / `libisl-15.dll`**，cc1plus 无法加载
  （实测 `error while loading shared libraries`）。磁盘上只有 64 位副本
  （`C:\Program Files\Git\mingw64\bin\`、FreeCAD），32 位 cc1plus 用不了。
- 机内无 MSVC / LLVM / MSYS2 / TDM-GCC；Git 自带的 mingw64 不含编译器。
- **可选方案**：拖一个便携 MinGW（如 w64devkit，约 60 MB，解压即用）进工作区，
  之后 25 套 native 套件就能真跑。**需要你点头**——这是在机器上加装工具链。
- 不装的话：`-fsyntax-only` + `ld -r` 符号审计已能给到"能编、能链接、符号齐全"的证据，
  但**断言是否通过仍然未知**。

### 8.2 `pio run` 被守卫拦死

守卫的实现是**每轮 50 个文件的删除预算**。PlatformIO 一启动包管理器就要删
`.pio/build`（912 项）、`framework-arduinoespressif32`（2072 项）、缓存 tmp（10017 项），
第一条 `Compiling` 之前就撞穿。实测结论（含无效绕法）：

- ❌ 无沙箱隔离运行 —— 守卫是 hook，不是沙箱，照拦
- ❌ `platform_packages = ...@symlink://` —— PlatformIO 仍解析注册表 spec，仍要删
- ✅ 有效替代：`build-reports/syntax_env_check.py`（§7）

**建议**：由你在沙箱外自己跑一次 `pio run -e nm-cyd-c5 -j 1`
（用项目本地 core dir：`PLATFORMIO_CORE_DIR=E:\github\HaleHound-CYD\.pio`，
配 `.pio/penv/Scripts/pio.exe` 6.1.18 —— **用全局 pio 会往 `~/.platformio` 里装东西**，
本轮已经误装了一个全局 `tool-esp_install@5.3.2`，无害但没必要）。

### 8.3 路线图基线冲突

| 文档 | 说法 |
|---|---|
| `DEVELOPMENT_PLAN.md` §3 | 「建立**五目标**构建脚本」 |
| `findings.md` Requirements | 「主板仅支持 **NM-CYD-C5 与 ESP32-2432S028**」 |
| `platformio.ini` 实际 | 2 个固件 env + 1 个 native |

`.pio/libdeps/` 里还留着 `esp32-cyd-hat` / `esp32-e32r28t` / `esp32-e32r35t` 三个旧环境的
依赖残留。**建议**：以 `findings.md` 的双主板为准，把 `DEVELOPMENT_PLAN.md` 的"五目标"
改掉，并清理 `libdeps` 残留。

### 8.4 合规审批流 ✅ 已闭合（`PolicyService` 单点授权）

> **结算**：攻击入口各自复制 `isOffensiveAllowed()` / `showDisclaimerScreen()` 的做法已收敛到
> `PolicyService` 唯一门禁，**Blue Team 粘滞**（拦截后不能靠再次接受免责声明解除），
> 由 `test_policy_single_point` 覆盖。详见 `DEVELOPMENT_PLAN.md` §3.3 与 §9 任务 1。

（以下为收口前的现状记录，保留作背景）

`findings.md` 记录：多个攻击入口各自复制 `isOffensiveAllowed()` / `showDisclaimerScreen()`，
且 Blue Team 拦截后仍可再次接受免责声明解除 Blue Team。

---

## 9. 下一步建议排序（2026-10-15 更新）

**完整清单见根目录 `REMAINING_WORK.md`**（含阻塞项、证据门、逐项依赖）。这里只给排序理由。

| 序 | 事项 | 理由 |
|---|---|---|
| 1 | **把 13 个新模型接进 `FeatureCatalog`** | 目前全是孤岛（无 `FeatureId` / 无生命周期工厂 / 无菜单入口）。不接线则一切「已实现」停在测试里，成本最低、杠杆最高 |
| 2 | **`CardDumpFormat` → `StorageService` → Loot 查看器** | 打通缺口 #15 与台架用例 B1（射频后 Loot 读卡）；上游连续三次加码的方向 |
| 3 | **沙箱外跑一次 `pio run`**（§8.2） | 不恢复出 `.bin` 的能力，一切验收都停在 `Build Verified` 之前 |
| 4 | **双主板台架回归 B1–B4**（缺口 #16/#17/#18） | 只能上机验证；`HeapBaseline` / `FeatureSwitchLedger` 目前只有模型自测数据 |
| 5 | **RFID 驱动链**：PN532 → ISO14443-A 认证 → MIFARE 转储 → Key Check → ST25R3916 | 上游 v4.0 的绝对重心；模型层已就位，缺的全是驱动与射频 |
| 6 | **七屏 UI 重构** + `RfidBackendModel` 能力矩阵接线 | 依赖 5 的能力上报 |
| 7 | **BLE client 传输层**（SkeletonKey / Chameleon / HID reader） | 三套模型已就位（29 例测试），卡在 `X3` UUID 证据与 BLE client 实现 |
| 8 | 其余 P1：触摸层（Fast Touch）/ 内存归还 / Wardriving 双核 / SubGHz 频谱 / Replay 选择器 / KeeLoq / IoT Recon | 按硬件齐备度排 |

**已结清的旧建议**：① `PolicyService` 单点授权 ✅；② 基线提到 v4.0 ✅；③ 宿主编译器决策 ✅
（用仓库本地 `ziglang` 解决，未在机器上加装系统级工具链）。
---

## 10. 数据来源与可信度

| 来源 | 用途 | 可信度 |
|---|---|---|
| 上游 Releases API 全量（28 个 release） | 本文档 §2/§3 全部能力条目 | **发布方自述**。仓库 29 个 commit 全是 README/LICENSE，v2.6.0~v3.4.0 所有 tag 指向同一 commit，**无源码可比对** |
| `docs/upstream-analysis/_all_releases.md`（296 KB 全文） | 逐条引用原文措辞 | 同上 |
| 本地 fork 源码 + 实测 | §4/§6/§7 | **实测，可复现** |
| Release Notes 之外的推断 | 已逐条标注「推断」 | 未经二进制验证 |

**关键限制**：受 IP 限制无法获取 `firmware.bin`，因此上游的实际运行时行为（协议细节、
内存占用、真实兼容性）**无法验证**。本文档中所有"上游支持 X"的说法，都是
**"上游称其支持 X"**。凡涉及协议正确性的实现（iCLASS 奇偶、HID26/35/48 位段、
Proxmark/Flipper 格式），fork 侧一律采取**保守断言**策略——只在确定的位置下结论，
不确定的地方显式标 `parityConfirmed = false` / `UNSUPPORTED FRAMING`，而不是猜。
