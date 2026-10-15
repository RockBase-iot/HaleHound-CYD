# HaleHound-CYD PlatformIO 重构与 v4.0 开发规划

| 项 | 值 |
|---|---|
| 需求基线 | **上游 v4.0**（2026-10-15 发布） |
| 上一版基线 | v3.8.1（2026-07-27）——已作废，落后 11 个版本 |
| fork 最后实现版本 | v3.4.0（2026-08-03，commit `b329e83`） |
| 落后跨度 | v3.5.0 → v4.0，18 个版本 / 16 次功能更新 |
| 基线依据 | `docs/upstream-analysis/RELEASE_BRIEF_zh.md`（上游 28 个 Release 全量梳理 + 35 项差距矩阵） |

> **变更记录（本次基线提升）**
> 1. 需求基线由 v3.8.1 一次性提到 **v4.0**。v3.8.1 是 2026-07-27 的快照，上游此后又发了
>    11 个版本，其中 v3.9.x 全是稳定性收口、v4.0 是「夯实底座 + RFID 大重构」。
> 2. 上游是**零源码分发仓库**（29 个 commit 全是 README/LICENSE），因此本规划的所有功能条目
>    只能来自 Release Notes，属于「上游称其支持」，不是可比对的事实。
> 3. 板目标由「五目标」收敛为 **双主板**（`esp32-cyd` + `nm-cyd-c5`），与 `findings.md`
>    已拍板的口径一致；上游的 8 个板目标不作为缺口。
> 4. 新增第 9 节「台架回归」与第 3.4 节「授权单点」，二者是本轮的实际交付入口。

---

## 1. 文档目的

本文档用于指导 HaleHound-CYD 从当前根目录平铺的 Arduino 工程，迁移为标准 PlatformIO 工程，
并以**上游最新 v4.0** 为产品需求基线。

本次工作必须分成三个独立但连续的交付流：

1. **工程重构**：先恢复全部目标的可重复构建，再迁移目录、入口、板级配置、菜单和模块生命周期。
2. **需求实现**：在稳定架构上补齐 v4.0 功能，并通过编译、台架、射频和安全合规验收。
3. **台架回归**：把「射频后 Loot 读卡」「BLE 模块任意顺序切换白屏」「heap 基线」这三项
   逐版本回归的稳定性问题，固化成可重复执行的台架用例，而不是每次靠人工复现。

不得在一次大提交中同时搬迁全部文件、重写菜单和新增攻击模块。每一阶段都必须保持至少一个
主目标可构建，并设置明确回滚点。

---

## 2. 当前基线结论

### 2.1 已确认事实

- fork 工作区固件版本为 `v3.4.0`，版本定义位于 `cyd_config.h`；上游已到 `v4.0`。
- 需求基线资料为 `docs/upstream-analysis/`（上游 28 个 Release 原始 JSON + 296 KB 全文 +
  中文梳理文档）。**仓库内没有、也拿不到 v4.0 源码或 `firmware.bin`**（IP 限制）。
- `platformio.ini` 已是标准布局（`src_dir = src`），默认环境 `nm-cyd-c5`。
- 工程已形成分层结构：`src/core`（策略/注册表/运行器/导航）· `src/board` · `src/features`
  · `src/drivers` · `src/platform` · `src/services/pure`。
- `src/app/LegacyApp.inc` 仍是巨型单 TU（约 4 800 行），承载菜单数据、渲染、触摸分发和
  模块生命周期；`src/app/App.cpp` → `AppKernel` 已是薄入口，可继续渐进替换。
- `SpiBusService` / `ResourceBroker` / `PeripheralRegistry` / `StorageService` /
  `BootHealthService` 已落地为纯逻辑 + 平台后端两层。
- **`PolicyService` 已实现单点授权**（本轮）：三态 `Undecided / ValhallaAccepted / BlueTeam`，
  Blue Team 粘滞、`acceptValhalla()` 在 Blue Team 下拒绝、唯一出口为
  `leaveBlueTeam(acknowledgedRisk=true)`；全部 21 处攻击入口已收敛到 `requestOffensiveAccess()`。
- `test/native/` 下共 30 套 Unity 套件（本轮新增 5 套：`test_policy_single_point`、
  `test_sd_handoff`、`test_ble_switch_ledger`、`test_heap_baseline`，另修正 `test_settings_policy`）。
- 本机**没有可用的宿主 C++ 编译器**，`native` 环境从未真正执行过（见 §5.4）。

### 2.2 构建与验证基线（2026-10-15）

| 环境 | 结果 | 备注 |
|---|---|---|
| `nm-cyd-c5`（RISC-V / IDF 5.5 / Arduino 3.3.6） | **可达** | 真实 env 语法校验全绿（见 §7） |
| `esp32-cyd`（Xtensa / Arduino 3.3.11） | 受限 | 本地 core dir 里的 classic 框架布局与手写 include 集不匹配，出现 `'assert'/'errno' was not declared`；**未改动文件报同样错误**，判为工具噪声 |
| `native` | 无法执行 | 缺宿主 C++ 编译器 |
| `pio run`（任一固件目标） | **结构性不可用** | 沙箱守卫的「每轮 50 文件删除预算」在第一条 `Compiling` 之前就被 pio 包管理器撞穿 |

因此，**本文档不把「全部支持板已可构建」当作发布事实**。当前可用的最强证据是
`build-reports/syntax_env_check.py` 在真实 env 下对齐 ESP-IDF + Arduino 3.3.x 的完整 include
集与宏定义所做的 `-fsyntax-only` 校验（**未覆盖链接、`extra_scripts`、`build_src_filter`**）。

### 2.3 v3.8.1 基线遗留项（不再作为基线，仅用于收敛）

| v3.8.1 条目 | 现状 | 处理方式 |
|---|---|---|
| WiFi 八项能力 | 已有 | 重构后做回归和门禁审计 |
| BLE Cinder（原 `BLE Jammer`） | 已有 | 统一产品命名 |
| BLE Predator | 缺失 | 新模块，拆分 SCAN/RECON/HONEYPOT；敏感数据写入必须显式授权 |
| WhisperPair | 已有 | 验证 CVE 声明、目标识别、失败路径和日志脱敏 |
| Airoha RACE | 缺失 | 独立研究任务；先验证公开技术依据 |
| Lunatic Fringe / AirTag 套件 | 已有或部分 | 按新菜单树合并 |
| NRF24 六项能力 | 已有 | `WLAN Jammer` → `WLAN Ember` 前先确认行为一致 |
| Tesla Charge | 缺失 | 仅在合法授权和射频合规门禁通过后排期 |
| `.Sub Read` | 缺失 | 优先实现只读解析、校验和预览 |
| NTAG Tools | 仅识别 | 补页读取、NDEF、写入、锁定和密码工作流 |
| Full Spectrum | 已有 | 验证三无线电资源切换、采样准确性和退出清理 |
| Flock You | 缺失 | 被动检测；OUI/GATT 数据必须可维护可追溯 |
| Drone Detect / RID Spoof / Jelly / Deauth | 缺失 | 先被动检测，主动项单独门禁 |
| Skimmer Hunt | 缺失 | 先定义评分模型、主动确认范围和误报验收数据集 |

### 2.4 v3.9.1 → v4.0 增量（本次基线提升的主体）

**v3.9.x —— 稳定性收口（上游把重心从「堆模块」转到「夯实底座」）**

| 版本 | 增量 |
|---|---|
| v3.9.1 | NRF 调参面板（SPI clock / 真实 dBm / 模式）；2.4G 干扰加密；KeeLoq 滚动码解码；BLE 名称中文化 |
| v3.9.2 | SubGHz 调参驾驶舱；NRF 面板按 jammer 显隐控件（**无死旋钮**）；Chameleon 自动连接 |
| v3.9.3 | 蓝牙工具可无限串联；**归还约 15 KB RAM**；低内存优雅退出 |
| v3.9.4 | WiFi 工具归还射频内存；Loot 查看器扩到 Honeypot / FlockYou / Predator |
| v3.9.5 | KARR 检测与脆弱性评估；Replay 目标选择器（7 类预设）；KeeLoq RF600E/FM 读取器；电台扫描器厂商名（4 万条）；GATT 全量落盘；**Chameleon 诚实报告 N of 16**；**移除 868/915/925 死频段** |

**v4.0 —— RFID 重构 + 底座工程化（当前最新基线）**

| 类别 | 增量 |
|---|---|
| RFID/NFC | ST25R3916 驱动 + RFID 七屏重构；**iCLASS 读/解/克隆**（26/35/48-bit + Corporate 1000）；Key Check（29 公开密钥）；MIFARE Classic 全卡转储 / 扇区尾访问位解码 / 带密钥写入 / 魔术卡；**Flipper `.nfc` + Proxmark `.eml` 双向**；ISO-DEP 停止协商 424 kbps → Type 4 全链可用 |
| BLE | KARR 新增 `0xBB Counter` 与 `0x55` 加密 framing，不支持时**明说 framing 不支持而非报「设备静默」**；**HID reader tooling**（Signo / iCLASS SE 定位与压测） |
| SubGHz | KeeLoq 要求解密功能码匹配明文按键才采信；Nice Flor-S 滚动码解码 |
| WiFi / IoT | IoT Recon 成长（DVR/GoAhead 凭据利用、修 RTSP digest、修 HTTP 路径、telnet）；FlockYou 加 Axon / Utility 执法记录仪 |
| Drone | 按目标选择；RID 假蜂群基于真实 GPS + 250 m 系绳；可滚动详情 |
| 硬件 | **Rd-01 24 GHz 雷达**；新增 AWOK Dual Touch v3 / AWOK MINI / E32R40T（fork **有意不跟**，见 §2.5） |
| 底座 | **One NDEF implementation**（4 处解码器合一）+ **50 例宿主测试**；**RAM 基线覆盖全部板目标**；出厂冒烟测试清单；SD 20 MHz；渐变条批量 `fillRect` + 动画门控 30 fps |

> 完整逐条清单与原文措辞见 `docs/upstream-analysis/RELEASE_BRIEF_zh.md` §2–§3。

### 2.5 有意偏离（长期保持，不作为缺口）

| 项 | 上游做法 | fork 决定 |
|---|---|---|
| VALHALLA = 擦除 SD | v3.2.0 起把「责任确认」与「擦数据」绑在同一个动作 | **禁止隐含删除**。政策面板只改授权状态；擦除独立命名为 Scorched Earth，放 Tools 菜单并自带 hold-to-confirm |
| Blue Team 退出方式 | 再次接受免责声明即解除 | **Blue Team 粘滞**。唯一出口是 Settings → Device Policy → 走独立确认的 `leaveBlueTeam(true)` |
| 板目标数量 | 8 个 | 2 个主板 + 可选扩展外设（按 `BoardProfile` / `ExpansionRoute` 扩展） |
| BLE Ducky | v3.5.0 已移除 | 不补 |
| 上游闪存包分发体系 | 按板分发 `.bin` | 不适用（自用板） |

---

## 3. 目标架构

### 3.1 目录结构

```text
HaleHound-CYD/
├── platformio.ini
├── boards/
│   ├── _jsonfiles/             # PlatformIO board JSON
│   └── NM-CYD-C5/              # variant 与板级定义
├── include/
│   ├── app/                    # 应用公共契约
│   ├── board/                  # BoardProfile、ExpansionRoute、引脚和能力定义
│   ├── core/                   # FeatureRegistry、PolicyService、Navigation、BootHealth
│   ├── drivers/                # CC1101 / NRF24 / PN532 / IR / RF433 / GPS
│   ├── features/               # 功能目录与各域纯逻辑模型
│   ├── platform/               # 执行策略与平台后端接口
│   └── services/               # SPI/资源/存储/显示/触摸/设置 + pure/ 纯逻辑
├── src/
│   ├── main.cpp                # 只保留 setup/loop 和 App 调用
│   ├── app/                    # App、LegacyApp 适配层
│   ├── board/                  # 各板编译期 BoardProfile
│   ├── core/
│   ├── drivers/
│   ├── features/
│   ├── legacy/                 # 尚未迁移的旧模块（逐步清空）
│   ├── platform/
│   └── services/
│       └── pure/               # 无 Arduino 依赖，可 native 测
├── test/
│   ├── native/                 # 纯逻辑宿主测试（Unity）
│   └── embedded/               # 设备侧冒烟测试
├── scripts/                    # 构建矩阵、宿主跑测器、补丁脚本
└── docs/                       # 架构、上游分析、台架记录、硬件验收
```

### 3.2 核心边界

#### App 与导航

- `main.cpp` 只创建 `App`，调用 `App::setup()` 和 `App::loop()`。
- 菜单项由 `FeatureDescriptor` 注册，不在触摸处理函数中维护大型 `switch`。
- 导航使用显式状态：`Boot`、`MainMenu`、`SubMenu`、`Feature`、`Dialog`、`Locked`。
- 保留现有阻塞式模块的兼容适配器（`LegacyFeatureAdapter`），逐步迁移为单次 `tick()`。

#### 模块契约

```cpp
struct FeatureContext;

class Feature {
public:
    virtual ~Feature() = default;
    virtual bool begin(FeatureContext& context) = 0;
    virtual void tick() = 0;
    virtual bool exitRequested() const = 0;
    virtual void end() = 0;
};
```

- `begin()` 失败必须返回错误，不得进入半初始化状态。
- `end()` 必须幂等，负责关闭任务、释放 BLE/WiFi、取消 SPI 占用并恢复显示状态。
- 旧 namespace 模块先通过 `LegacyFeatureAdapter` 接入，验证通过后再内部重构。

#### 板级配置

- 每个环境只选择一个 `BoardProfile`，包含显示、触摸、SPI、外设能力和板名。
- 禁止在业务模块内继续增加板型 `#ifdef`；差异通过能力查询或驱动实现处理。
- C5 与经典 ESP32 可使用不同显示后端，但向 UI 暴露同一接口。
- 扩展外设（CC1101 / NRF24 / PN532-I2C / IR / RF433 / SD / GPS）是**独立最小外设单元**，
  通过 `PeripheralRegistry` 运行时探测；未找到时显示「设备未找到」而不是构建失败。

#### 资源管理

- `SpiBusService` + `ResourceBroker` 提供作用域租约，租约析构时自动释放。
- 显示渲染与 SD、射频设备同处一个 SPI 仲裁域：**禁止业务层直接访问 `SPI` / TFT / SD**。
- 模块退出后必须把共享总线交还给仲裁器，且**不得留下未挂载的 SD 卷**（见 §3.4）。

#### 设置与持久化

- 使用带 `magic`、`schema`、长度和 CRC 的双槽记录（`SettingsRecord`）。
- 政策状态（`PolicyService`）使用**独立的双槽 + CRC 区域**（EEPROM offset 128），不与
  旧 `Settings` blob 混用；首次启动从旧字节一次性迁移。
- 旧 EEPROM 数据提供一次迁移，失败时**fail-closed 到 `Undecided`**（仍然锁住攻击性功能）。

### 3.3 授权单点（PolicyService）

上游的 VALHALLA 语义在 fork 中重写为「三态 + 粘滞 + 唯一出口」：

```text
Undecided        --acceptValhalla()--> ValhallaAccepted
Undecided        --declineValhalla()--> BlueTeam
ValhallaAccepted --enterBlueTeam()-->  BlueTeam   (保留 disclaimer 位)
BlueTeam         --acceptValhalla()--> REFUSED    (粘滞，这就是被堵住的口子)
BlueTeam         --leaveBlueTeam(true)--> ValhallaAccepted
any              --lockDown()-->        BlueTeam + 清 disclaimer 位
```

- `authorize()` 返回 `{code, action, detail}`，`action ∈ {Proceed, PromptValhalla,
  BlockedBlueTeam}`。调用方**不再自己判断该弹哪个屏**。
- `BlockedBlueTeam` 分支**绝不提供免责声明入口**：Blue Team 只有一个出口，且是独立流程。
- 应用层唯一门禁：`requestOffensiveAccess()`（`src/app/LegacyApp.inc`）。21 处旧复制门禁已全部替换。
- 用户入口：主屏底部模式条点击 → **Device Policy 面板**（接受 / 留在 Blue Team / 离开 Blue Team），
  以及 Settings → `Device Policy`。两者走同一实现。
- 擦除 SD 的 `scorchedEarthWipe()` 已从模式条点击移出，只保留在 Tools → `Wipe SD Card`。

**验收**：任意入口（菜单、深链、功能间交接）都无法在 Blue Team 下启动主动发射功能；
`test_policy_single_point` 的 11 个用例覆盖该不变量。

### 3.4 台架回归模型（双主板）

三个问题都曾在上游连续三个版本被反复修（SD：#16；BLE 白屏：#17；内存归还：#18），
说明它们无法靠读代码确认。fork 侧把它们做成**纯逻辑模型 + 可重复台架用例**：

| 模型 | 文件 | 固化的问题 |
|---|---|---|
| `SdHandoff` | `include/services/SdHandoff.h` + `src/services/pure/SdHandoff.cpp` | 射频会话结束后卷未重新挂载，Loot 报「空目录」而不是 `Busy`/`Remounted` |
| `FeatureSwitchLedger` | `include/services/FeatureSwitchLedger.h` + `src/services/pure/FeatureSwitchLedger.cpp` | 任意顺序切 BLE 模块时的显示饥饿（白屏）、BLE 栈未 teardown、租约泄漏；对 5 个模块的 **120 种排列**逐一校验 |
| `HeapBaseline` | `include/services/HeapBaseline.h` + `src/services/pure/HeapBaseline.cpp` | free / minFree / largestBlock 相对基线的整型百分比分级（8 % warn、15 % critical、24 KiB 绝对地板、碎片率） |

台架操作流程与判定口径见第 9 节。

---

## 4. 实施阶段

### 阶段 0：恢复可重复构建（P0）

**工作项**

- 固定 PlatformIO 平台、框架和库的准确版本，避免自动升级改变结果。
- 解决 `esp32-cyd` 环境在本地 core dir 下的 classic 框架布局问题（或在文档中明确其校验边界）。
- 建立**双目标**构建脚本，记录 Flash/RAM 用量和固件 SHA256。
- 清理 `platformio.ini` 中已不支持的旧环境残留与 `.pio/libdeps/` 旧环境目录。

**验收标准**

- `nm-cyd-c5` 与 `esp32-cyd` 均可从清理后的缓存完成编译；若某一目标暂列实验，需在文档中显式声明。
- 连续两次构建结果一致，不依赖手工修改第三方库。
- 构建警告形成基线，新增警告在后续阶段视为失败。

### 阶段 1：建立标准 PlatformIO 骨架（P0）✅ 基本完成

- `src/`、`include/`、`test/` 已建立，入口已转为 `main.cpp` → `App` → `AppKernel`。
- 遗留项：`src/app/LegacyApp.inc` 仍需按域继续切分。

### 阶段 2：板级配置与驱动解耦（P0）🟡 进行中

- `BoardProfile` / `ExpansionRoute` / `PeripheralRegistry` 已落地。
- 遗留项：业务模块中仍有直连 `SPI` / TFT 的路径，需按阶段 3 清理。

### 阶段 3：资源和生命周期治理（P0）🟡 进行中

**工作项**

- 迁移所有直接 `SPI.begin/end`、CS 写入和跨设备切换到 `SpiBusService`。
- 统一 WiFi/BLE 初始化与清理；**禁止会破坏再次初始化的 BLE 释放方式**（v3.7.2/v3.8.1/v3.8.2/v3.9.3 反复修的就是这一类）。
- 把模块退出清理纳入统一执行器，即使 `begin()` 失败也能恢复资源。
- 接入 `SdHandoff`：射频会话前后由服务层统一管理 SD 卷挂载状态。

**验收标准**

- 任意时刻共享 SPI 最多一个设备被选择。
- 模块连续进入/退出 20 次，无死锁、无明显堆内存持续下降。
- Radio Test、SD 读写、PN532、NRF24、CC1101 可依次执行并成功恢复。

### 阶段 4：菜单与模块注册表（P1）🟡 进行中

- `FeatureRegistry` / `FeatureCatalog` 已就位（21 条描述符）。
- VALHALLA/Blue Team 规则已收敛为 `PolicyService` 单点授权（本轮完成）。
- 遗留项：菜单树仍由 `LegacyApp.inc` 的数组维护，需迁到注册表；返回项不再占用业务索引。

### 阶段 5：现有模块分域迁移（P1）

按依赖和风险执行：

1. Tools、Settings、About。
2. Jam Detect 和被动扫描模块。
3. RFID、SubGHz、NRF24 等单外设模块。
4. WiFi、Bluetooth。
5. SIGINT 和跨模块链路。

每迁移一个域，都要完成构建矩阵、菜单启动、退出清理和存储回归。

### 阶段 6：v3.9.x 稳定性与体验项（P1）

对应上游 v3.9.1–v3.9.5：

- NRF / SubGHz 调参驾驶舱（`RadioTuningModel` 已就位，缺 UI 接线）。
- BLE 名称数据库（厂商 / 服务 UUID / GAP 外观 → 真实名）。
- 内存归还与低内存优雅退出（**必须由 `HeapBaseline` 给出前后百分比，不接受「感觉省了」**）。
- Chameleon 自动连接 + 诚实报告（`ChameleonUltraModel` 已就位）。
- Replay 目标选择器；移除死频段（已在模型中固化 `offered=false`）。
- Loot 查看器扩展到 Honeypot / FlockYou / Predator。

### 阶段 7：v4.0 RFID 主战场（P0/P1）

- **统一 NDEF 实现**（`NdefCodec` 已就位 + 8 例测试；上游为 50 例，需补到同等覆盖）。
- **卡转储格式双向**（`CardDumpFormat` 已就位）。
- ST25R3916 驱动 + RFID 七屏重构。
- iCLASS 读/解/克隆（`IclassPacs` 已就位，缺射频与 3DES 路径）。
- MIFARE Classic 全卡转储 / 密钥写入 / 魔术卡 → 依赖它再做 Key Check（29 密钥）。
- ISO-DEP / Type 4 全链。
- NTAG Tools 写入侧（擦除 / 永久只读 / 设密码 / 清密码）。
- RFID dump 落 SD 并可机内查看（对接 `StorageService`）。

### 阶段 8：高风险功能研究与实现（P2，独立审批）

- BLE Predator、Airoha RACE、Tesla Charge、RID Spoof、Drone Jelly、Drone Deauth。
- IoT Recon 的主动利用路径（DVR/GoAhead 凭据利用、telnet）。
- 任何干扰、凭证捕获、密钥提取或主动仿冒能力。

每项必须先提交威胁模型、授权使用场景、地区法规限制、射频功率限制、日志数据分类和禁用机制。
研究结论不足时，README 必须降级为「计划中」或移除声明，**不能用占位实现冒充完成**。

### 阶段 9：发布工程（P1）

- 版本号单一来源，同步固件、菜单、构建产物和 README。
- 为每块板生成命名明确的 `.bin`、构建清单、SHA256 和刷写地址。
- CI 构建矩阵、格式检查、主机测试和固件尺寸预算。
- 建立硬件验收记录；README 的 "Fully Tested" 只能来自有日期和板卡版本的测试结果。

---

## 5. 测试策略

### 5.1 可自动化测试（宿主 / native）

- 解析器类：`.sub`、NDEF、Remote ID、BLE 广告、EAPOL、日志格式、卡转储格式。
- 决策类：`PolicyService` 授权、`FeatureCatalog` 注册、`MenuLayout`、`NavigationController`。
- 资源类：`ResourceBroker` / `SpiBusService` / `PeripheralRegistry` / `StorageService`。
- **台架回归类**：`SdHandoff` / `FeatureSwitchLedger` / `HeapBaseline`（§3.4）。
- 每次合并运行全部环境构建，并比较 Flash/RAM 是否超过预算。

### 5.2 双主板硬件测试矩阵

| 测试域 | `esp32-cyd`（经典 ESP32） | `nm-cyd-c5`（单核 RISC-V） |
|---|---:|---:|
| 显示 / 旋转 / 颜色 / 反色 | 必测 | 必测 |
| 触摸校准与命中 | 必测 | 必测 |
| SD 读写（含射频后读卡） | 必测 | 必测 |
| CC1101 | 必测 | 必测（拨码 1） |
| NRF24 | 必测 | 必测（拨码 2） |
| PN532 | SPI 单独测 | I2C 单独测（拨码 3） |
| IR / RF433 | 必测 | 必测（拨码 4 / 5） |
| GPS / USB 串口恢复 | 必测 | 必测 |
| 连续模块切换（含 BLE 任意顺序） | 必测 | 必测 |
| heap 基线采样 | 必测 | 必测 |

> 拨码说明：NM-RF-HAT 的 1–5 位任意时刻只能有一位 ON，因此**物理上不可能同时发现全部外设**；
> 运行时探测必须按需探测 + 状态缓存，未找到时同时提示检查拨码位置、供电和连接。

### 5.3 台架回归用例（必须每次发布前跑）

| # | 用例 | 通过标准 |
|---|---|---|
| B1 | **射频后 Loot 读卡**：进入 RFID/SubGHz/NRF → 退出 → 打开 Loot | 读到卡内既有文件；**不得出现空目录**。控制台出现 `SdCode::Remounted` 或 `Ready`，不得出现 `Busy`／`NotMounted`（除非确实无卡） |
| B2 | **BLE 模块任意顺序切换**：Jammer / Spoofer / AirTag hub / Scanner / SkeletonKey 任取 5 种顺序各跑一遍 | 屏幕始终有内容（无白屏/无残留最后一帧）；每次退出后 heap 不持续下降；`FeatureSwitchLedger::replay()` 对 120 种排列返回 `Clean` |
| B3 | **heap 基线**：冷启动后静置 60 s，再逐模块进出后各采一次 | `HeapBaselineCheck::assess()` 全部 `WithinBudget`；相对基线回退 ≥ 8 % 记 warn，≥ 15 % 或 free < 24 KiB 记 critical |
| B4 | 连续运行 30 分钟 | 无看门狗复位，关键循环无持续堆下降 |

### 5.4 当前无法执行的验证（已知边界）

| 项 | 原因 | 影响 |
|---|---|---|
| `pio run` 真实链接 | 沙箱守卫「每轮 50 文件删除预算」被 pio 包管理器撞穿 | 无法产出 `.bin`；用真实 env 的 `-fsyntax-only` 代替（不覆盖链接与 `extra_scripts`） |
| 上机执行 B1–B4 | 需要两台实机 + SD 卡 + 模块顺序切换 | `HeapBaseline` 目前只有模型自测数据，没有真实采样 |

**已闭合项**：宿主断言执行。`build-reports/hostenv` 内装了 `ziglang`（pip wheel，自带
clang 21 + libc++ sysroot），`scripts/hostcheck/run_host_tests.py` 自动探测它并编译 +
链接 + **执行**全部 native 套件。2026-10-15 实测：**36/36 套件全绿**
（29 套基线 + 本轮 RFID/BLE 新增 7 套；基线首次执行时为 142 个用例）。

剩余边界仍必须在进入阶段 9 之前闭合，否则「已实现」永远停在 `Build Verified` 之前。

### 5.5 首次宿主跑测暴露的潜伏缺陷（2026-10-15 已全部修复）

这些套件此前从未被编译执行过一次（仓库没有可用宿主编译器），首跑 20/29，9 个失败
全部是真实问题，不是环境噪声：

| 套件 | 症状 | 判定 | 处置 |
|---|---|---|---|
| `test_board_profiles` 等 4 套 | `TEST_ASSERT_EQUAL_UINT32` 未定义 | shim 能力缺口 | `scripts/hostcheck/unity/unity.h` 补齐类型化相等宏 |
| `test_spi_service` | `undefined symbol: WinMain` | **测试文件缺陷**：`main()` 被写在匿名 namespace 内，全局 `main` 符号根本不存在 | 在 `main` 前闭合 namespace，删掉多余的尾部 `}` |
| `test_feature_registry` | `incomplete type 'FeatureRegistry'` | **测试文件缺陷**：`FeatureCatalog.h` 只做前置声明 | 补 `#include "core/FeatureRegistry.h"` |
| `test_ble_switch_ledger` | `replay(nullptr, 0)` 被判定 `InvalidProfile` | **实现缺陷**：空访问序列应满足空真 | `count == 0` 直接返回 `Clean` |
| `test_card_dump_format` | `.nfc` 往返丢块；`uid[0]`、`unknownKeys` 均不符 | **实现缺陷 ×2 + 夹具缺陷 ×1** | 见下表 |
| `test_ndef_codec` | 文本记录负载 400 vs 403 | **断言缺陷**：`buildText` 会前置状态字节 + 语言码 | 改为断言往返字节相等，并显式断言 > 255 以锁定四字节长度形式 |

`CardDumpFormat` 的两个真实缺陷：

1. `toFlipperNfc` 写出的未读块占位 `Block n: ?? ?? … ??` **无法被自己的解析器读回** ——
   该行落入 `unknownKeys`，块槽位丢失、后续块号全部前移，等于静默损坏转储。
   已让 `parseBlockLine` 识别 `??` 占位并保留槽位（`known = false`）。
2. 解析器不认 `Version:` 与 `Data format version:` —— 这两个是它**自己写出来的**已知键，
   被当作 unknown 保留后，每次「存 → 读 → 存」都会在文件里重复累积。已显式消费。

夹具缺陷：`makeClassicDump()` 的 block 0 是 `0..15` 递增填充，却断言
`fromProxmarkEml` 能从中推出 UID `04 12 34 56`。MIFARE Classic 的块 0 是厂商块
（UID 字节 0..3 + BCC + SAK + ATQA），**实现是对的**，已把夹具改成真实厂商块布局。

### 5.6 模型层跑测暴露的缺陷（2026-10-15，RFID/BLE 差距项）

本轮新增 7 套测试后，36/36 全绿。但**先写模型后写测试**的模块里查出了 7 个真实缺陷，
其中 5 个只有真的跑断言才会暴露 —— 再次印证「代码看起来合理」不构成任何证据：

| 位置 | 缺陷 | 为什么 code review 看不出来 |
|---|---|---|
| `MifareClassicModel` 尾块权限表 | 把「仅 Key A」错实现成「Key A 或 Key B」 | 语义上「差不多」；**证据文档 §1.3 的表格本来是对的，是实现漂移了** |
| `GattNameTable` | 13 处名称错 + 7 个根本不该存在的条目，其中把**服务** `0x1813` 的名字安到了**特征** `0x2A31` 上 | 凭记忆写的 165 条表，看着完全合理 |
| `RfidBackendModel::screenLabel` | `BackendPicker` 与 `CardReader` 都返回 `"Reader"` | 七屏里两屏同名，顶栏无法区分当前在哪一屏 |
| `ChameleonUltraModel::evaluateRead(20, 16)` | 返回 `valid = true` + `"complete read"` | 读到的页数比卡上还多，是驱动计数错误，却给出肯定结论 |
| `ChameleonSession::onDeviceFound("")` | 采纳空地址，会话进 `Ready` 却没有可连对象 | — |
| `SkeletonKeyModel::classifyAppearance` | `0x03C3`(摇杆) / `0x03C5`(数字化仪) 报成 `Keyboard` | 正是该函数注释里说「绝不发明设备类型」要避免的行为 |
| `SkeletonSession::canWriteFullDump()` | 只查树不查状态；`requestStop()` 只改状态不清树 → **已停止的会话仍声称可写全量转储** | — |

`GattNameTable` 的解法值得固化：**不靠眼睛，拉 Bluetooth SIG 机读清单做 diff**，并把规则
设计成零例外 —— SIG 已分配必须逐字一致（SIG 官方就叫 `GAP` / `Battery`，不是
`Generic Access` / `Battery Service`），未分配必须带 `(vendor)` / `(member)` 标记，
无法核验的条目**删除**而不是写进例外清单。核验器：`build-reports/verify_gatt_names.py`。

---

## 6. 工作拆分与依赖

| Epic | 优先级 | 依赖 | 主要产物 |
|---|---|---|---|
| E0 构建基线 | P0 | 无 | 可重复的双目标构建 + 尺寸基线 |
| E1 PlatformIO 骨架 | P0 | E0 | `src/include/test` 结构（基本完成） |
| E2 BoardProfile | P0 | E1 | 板级配置与驱动边界（进行中） |
| E3 ResourceManager + SdHandoff | P0 | E1、E2 | SPI/Radio/UART/SD 租约与交接 |
| E4 App/Menu + PolicyService | P0 | E1、E3 | 状态机、功能注册表、**单点授权**（授权已完成） |
| E5 旧模块迁移 | P1 | E2、E3、E4 | 分域模块目录 |
| E6 台架回归 | P0 | E3 | B1–B4 用例 + `HeapBaseline` 报告 |
| E7 RFID 主战场（v4.0） | P0 | E3、E5 | ST25R3916 / MIFARE / iCLASS / NDEF |
| E8 v3.9.x 稳定性与体验 | P1 | E5 | 调参驾驶舱、内存归还、Loot 扩展 |
| E9 主动/高风险功能 | P2 | E3、E5、审批 | 独立模块与安全门禁 |
| E10 CI 与发布 | P1 | E0 起持续实施 | 构建矩阵、产物和验收记录 |

建议每个提交只完成一个可独立验证的纵向切片。

---

## 7. 当前验证手段与证据边界

**主校验器**：`build-reports/syntax_env_check.py`

- 直接读取 pioarduino 生成的权威 flags 文件
  （`framework-arduinoespressif32-libs/<chip>/flags/{includes,defines,cpp_flags}`），
  等于拿真实 ESP-IDF include 集 + 真实宏定义做 `-fsyntax-only`。
- 用 `-iprefix` + `-iwithprefixbefore` 压缩命令行，规避 Windows 32 KB 上限。
- 必须指向**项目本地 core dir 里对应版本的 core**：C5 用 `.pio/c5-core/packages/framework-arduinoespressif32`
  （Arduino 3.3.6），否则会误用 `.pio/packages` 里残留的 Arduino 2.0.17，导致每个 TU 都报
  「3.x 才有的头文件找不到」。
- **未覆盖**：链接、`extra_scripts`、`build_src_filter`、Flash/RAM 占用。

**次级校验器**：`scripts/hostcheck/run_host_tests.py` + `build-reports/hostenv`

- 用仓库本地 `ziglang` venv 驱动 `python -m ziglang c++`，行为等同 `clang++`。
  安装与调用方式见 `build-reports/README.md`。
- **先探针再采纳**：`~/.platformio` 里的 mingw32 包缺 32 位 `libgmp-10.dll` 等，
  `g++ --version` 正常但每次编译都以**完全空的日志**失败。因此每个候选编译器都要真的
  编译 + 链接 + 运行 `int main(){return 0;}` 才能被选中，否则静默降级到 zig。
- **共享源先归档再链接**：直接强制链接所有 `.o` 会因为 `src/core/AppKernel.cpp` 引用
  `build_src_filter` 排除掉的 `legacySetup` / `legacyLoop` 而报未定义符号。打成
  `libhostcheck.a` 才复刻 PlatformIO `native` 环境「只拉取被引用目标文件」的语义。
  **不要用空桩函数去「修」这个报错** —— 那会掩盖固件侧真实的链接错误。
- 输出每套件 `PASS/FAIL`，失败时给出断言的文件名、行号与期望/实际值。

**判定规则**：改动文件与同环境的**未改动文件**报同样的错 → 判定为工具噪声，不作为回归。

---

## 8. 完成定义

本次重构只有同时满足以下条件才算完成：

1. 工程使用标准 `src/`、`include/`、`test/` 结构，根目录不再参与源码通配编译。
2. 声明支持的全部 PlatformIO 环境可重复构建，并有 CI 记录。
3. `main.cpp`、板级配置、资源管理、菜单和业务模块之间边界清晰。
4. 所有模块遵守统一生命周期，退出后不遗留 SPI、WiFi、BLE、UART 或未挂载 SD 卷。
5. **授权单点闭环**：唯一门禁入口，Blue Team 无法从任何路径被绕过，政策状态只有
   `PolicyService` 写。
6. **台架回归 B1–B4 全部通过**，且 heap 相对基线无 critical 回退。
7. v4.0 每项功能都有对应实现、状态和验收证据；未实现项不再以已发布能力描述。
8. 每块标记为支持的硬件都有完整测试记录和可追溯固件产物。
9. 主动发射和敏感数据功能通过授权、法规和安全门禁审查。

---

## 9. 第一批可执行任务

| 序 | 事项 | 理由 |
|---|---|---|
| 1 | ✅ **`PolicyService` 单点授权收口** | 合规 P0；21 处门禁已收敛，Blue Team 粘滞 |
| 2 | ✅ **基线提到 v4.0**（本文档） | 否则每补一版都再落后一次 |
| 3 | 🟡 **双主板台架回归 B1–B4** | 模型与用例已就位；缺上机执行 + `HeapBaseline` 实测数据 |
| 4 | 🟡 **闭合 §5.4 验证边界**：宿主编译器 ✅ 已闭合；沙箱外 `pio run` 待办 | 不闭合则一切「已实现」都停在 `Build Verified` 之前 |
| 5 | ✅ **主机跑测**：**36/36 套件全绿**（基线首次执行时 29/29、142 个用例；本轮新增 7 套）。累计修掉 9 个从未被编译过的潜伏失败（§5.5）+ 7 个模型层真实缺陷（§5.6） | 断言级验证已可用 |
| 6 | **Loot Manager + StorageService 对接** | 把 `CardDumpFormat` 的 `.nfc`/`.eml` 落盘并可机内打开；同时是 B1 的落点 |
| 7 | **RFID 主动路径**：PN532 驱动 → MIFARE Classic 转储 → Key Check → ST25R3916 | 上游 v4.0 的绝对重心。模型层已就位（`MifareKeys` / `MifareClassicModel` / `KeyCheckModel` + 29 例测试），**缺驱动层** |
| 8 | **`LegacyApp.inc` 分域切分**（Tools/Settings 优先） | 降低后续每项改动的回归面 |
| 9 | **把 13 个新模型接进 `FeatureCatalog`** | `NdefCodec` / `CardDumpFormat` / `IclassPacs` / `KarrModel` / `RadioTuningModel` / `SkeletonKeyModel` / `ChameleonUltraModel` / `HidReaderModel` / `RfidBackendModel` / `MifareKeys` / `MifareClassicModel` / `KeyCheckModel` / `GattNameTable` **目前全是孤岛**：无 `FeatureId`、无生命周期工厂、无菜单入口。不接线则一切「已实现」停在测试里 |
| 10 | 其余 P1：触摸层、内存归还、Wardriving 双核、KeeLoq、SubGHz 频谱、BLE 名称库 | 按硬件齐备度排 |

> 完整剩余工作清单（含阻塞项、证据门、建议执行顺序）见根目录 **`REMAINING_WORK.md`**。
