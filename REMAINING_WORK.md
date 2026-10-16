# 剩余开发工作清单（Remaining Work）

| 项 | 值 |
|---|---|
| 快照时间 | 2026-10-16 16:30 GMT+8 |
| 分支 | `nm-cyd-c5` |
| 快照依据 | 工作树全量宿主回归 **`49/49` 套件 / 446 用例**通过（`ziglang` clang 21）+ **运行时可达性只读走查**（§0.5，10 条硬证据）+ **外部证据复核**（§0.8，`docs/upstream-analysis/USER_EVIDENCE_zh.md`） |
| 上游基线 | v4.0（release notes 见 `RELEASE_BRIEF_zh.md`；**v4.0 许可已换为专有，v3.4.0 时代是 MIT**，见 §0.8 / §8） |
| 主计划 | `DEVELOPMENT_PLAN.md` 阶段 5–9 |

> **本清单只记「还没做」的事。** 已完成的模型层与测试覆盖见 `progress.md`；
> 差距分级（P0/P1/P2/D）口径见 `RELEASE_BRIEF_zh.md` §5。

---

## 0. 一分钟结论

- **可验证的逻辑层已经基本铺满**：RFID / BLE 的 13 个纯逻辑模型 + 40 套宿主测试全绿。
- **RFID 的 PN532 驱动链本轮已打通**：`IRfidTransport`（传输抽象）→ `Pn532Protocol`（主机帧
  编解码 + 状态码）→ `Pn532Transport`（I2C 命令时序）→ `Iso14443aFraming`（ISO14443-A 字节层
  + CRC_A）→ `MifareClassicDriver`（会话：激活/认证/转储/写前拒绝），每层一套独立宿主测试。
  ST25R3916 按 BOSS 指令**只做预留接口**，见 `docs/protocol-evidence/st25r3916.md`。
- **但 UI 与接线仍是孤岛**：13 个模型 **加上** 本次 5 组驱动文件全部没有注册进
  `FeatureCatalog` —— 无 `FeatureId`、无生命周期工厂、无菜单入口。当前状态是
  `Model Verified` + 部分 `Build Verified`，离 `Hardware Verified` 还差一层。
- **BLE 传输层（B1–B3）本轮已补齐**：`SkeletonKeyClient` + `GattTreeView`、
  `ChameleonFrame`（逐字节增量接收器）+ `ChameleonUltraClient`、`HidReaderClient`，
  各配一套宿主测试；协议数字逐条对过固件源码，记入 `docs/protocol-evidence/chameleon-ultra.md`。
  B5 的**宿主侧收口**也已就位（`BleVisitAudit`），只剩真机采样（`X2`）。
- **下一步最高 ROI**：① ~~新模型/驱动进 catalog~~ —— **见 §0.5：只进 catalog 不够，
  因为 runner 从来没有被启动过** → ① ~~`U1` 写 `Esp32BleCentral`~~ ✅（§0.6）→
  ~~`U2` 接线~~ ✅（§0.7：runner 已有真实调用点，policy 门已装，渲染钩子已通）→
  **真·① `U3`**：`ChameleonUltra` 补 `FeatureId` + descriptor，让它能出现在菜单上 →
  ② `U4`：`Pn532I2cDriver` 去留决策 → ③ `U5`：16 条空壳条目补实现或摘掉 →
  ④ `X2` 上机跑一次真实 BLE 连接（地址反转 / live 扫描两条结论待真机复验）。
- **外部证据新增了一条会决定「能不能发货」的轨道**：依赖里 `RF24` = GPL-2.0-only、
  `arduinoFFT` = GPL-3.0，**两者互不相容**⟹ 当前依赖集下固件二进制不可分发（`C1` / `X6`，见 §0.8 / §8）。

---

## 0.5 运行时可达性审查（2026-10-16，只读走查）

> **一句话：逻辑层扎实，但「整条新架构在运行时不可达」是当前唯一的 P0 级结构缺口。**
> 把「编译通过 + 宿主测试 48/48 绿」当成「能用」，是本项目此刻最大的认知陷阱。

| # | 事实 | 证据 |
|---|---|---|
| **A1** | ~~**`Esp32BleCentral` 不存在**~~ → **本轮已落地**（见 §0.6） | `include/drivers/Esp32BleCentral.h` + `src/drivers/Esp32BleCentral.cpp`：`BLEClient` + `BLEScan` + `BLERemoteCharacteristic`，覆盖 **NimBLE(C5) / Bluedroid 3.x / Bluedroid 2.0.17** 三种组合。**注意：它还没有被任何人实例化** —— B1/B2/B3 的底座写好了，但接线属于 `U2` |
| **A2** | **`FeatureRunner::start()` 全仓无调用点** | `grep -rn 'runner_.start\|\.start(FeatureId' src/` 为空（`PolicyService.cpp:76` 只构造了一个字面 descriptor）⟹ **catalog 全部 76 条运行时不可达** |
| **A3** | **`AppKernel` 给 runner 传的是空钩子 `{}`** | `src/core/AppKernel.cpp:14`。即便有人调 `start()`：`FeatureRunner.cpp:8` 的 `allowed()` 把**空回调视为放行**，于是 `PolicyService` 授权门、外设探测、资源预检全部被短路 —— **授权单点形同不存在** |
| **A4** | **`LegacyApp.inc`（4661 行）是唯一的运行时** | 它 grep 不到 `runner` / `featureCatalog` / `FeatureId`；全部功能直接调旧单体（`WifiScan::setup()` …） |
| **A5** | **新功能在 UI 里 0 个入口** | 对 `KARR / SkeletonKey / HID Reader / Predator / Airoha / Flock / Skimmer / Drone / Chameleon` 逐个 grep `LegacyApp.inc` **全部为 0**；蓝牙子菜单仍只有 10 条旧项 |
| **A6** | **`IFeature` 没有渲染钩子** | `include/core/IFeature.h`：`tick()` 返回 `void`，接口里没有任何 draw/display 概念 ⟹ 新屏（GATT 树、槽位表）**无处可画** |
| **A7** | **Chameleon Ultra 连 catalog 条目都没有** | `FeatureId` 枚举无 `BluetoothChameleonUltra`；`createChameleonUltraFeature()`（`ChameleonUltraModel.cpp:199`）**零引用** |
| **A8** | **PN532 四层驱动是封闭孤岛** | `Pn532Transport`/`Pn532Protocol`/`Iso14443aFraming`/`MifareClassicDriver` 只有互相引用 + 宿主测试；`CardDumpFormat` / `IclassPacs` / `NdefCodec` / `RfidBackendModel` / `RadioTuningModel` / `KarrModel` **生产侧零引用** |
| **A9** | **`Pn532I2cDriver` 已完全孤儿** | `src/drivers/Pn532I2cDriver.{h,cpp}` **全仓零引用**（连头文件都没人 include）⟹ `W6` 不是「收并」而是「删除或改为接管」 |
| **A10** | **16 条 catalog 条目是纯空壳** | `CatalogFeatureStub`（只有 begin/tick/end 的橡皮图章）：`SubGhzSavedProfiles` / `SigintWardriving` / 9 个 `Setting*` / 5 个 `Tool*` |

### 由此重排的紧急度

| 新 ID | 事项 | 为什么紧急 | 级别 |
|---|---|---|---|
| **U1** | ~~写 `Esp32BleCentral`~~ ✅ **本轮完成**：`src/drivers/Esp32BleCentral.cpp`（`BLEClient` + `BLEScan` + `BLERemoteCharacteristic`）+ `docs/protocol-evidence/ble-central-transport.md`。`nm-cyd-c5` / `esp32-cyd` / 宿主 三处全绿 | —— | ✅ 完成 |
| **U2** | ~~**把架构接通**~~ ✅ **本轮完成**（§0.7）：`AppKernel` 用 `PolicyService` 填 `FeatureRunnerHooks`（`authorize` 改为返回 `Authorization`，缺钩子时对 offensive 失败关闭）；`IFeature::draw(IFeatureCanvas&)` + `exitRequested()` + `AppKernel::setCanvas()`；蓝牙子菜单由 `FeatureMenuModel` 从 catalog 生成并经 `runner.start(id)` 派发 | —— | ✅ 完成 |
| **U3** | `ChameleonUltra` 补 `FeatureId` + descriptor | B2 连「注册」这步都没做 | **P0** |
| **U4** | `Pn532I2cDriver` 去留决策（倾向删除） | 同一条 I2C 总线上两套驱动的可能性是真实故障面 | P1 |
| **U5** | 16 条空壳条目：要么补实现、要么从 catalog 摘掉 | 「菜单里有、点了没反应」比「菜单里没有」更坏 | P1 |

---

## 0.6 `U1` 落地：`Esp32BleCentral`（2026-10-16）

**交付物**

| 文件 | 内容 |
|---|---|
| `include/drivers/Esp32BleCentral.h` | `IBleCentral` 的实现声明；单文件 `#ifdef ARDUINO` 守卫，与 `WirePn532Link` 同构 |
| `src/drivers/Esp32BleCentral.cpp` | `BLEDevice` / `BLEScan` / `BLEClient` / `BLERemoteCharacteristic` 全量实现 + 三方组合的 normalise 层 |
| `build-reports/syntax_env_check.py` | 加了 `Esp32BleCentral.cpp` / `BleCentral.cpp` 到默认集；**修好了 `esp32-cyd` 那个「基线也 FAIL」的假象** |
| `docs/protocol-evidence/ble-central-transport.md` | 两条 host 差异的逐条一手证据 + 两个静默陷阱 |

**关键发现（推翻了原设计前提）**

1. **主板 `nm-cyd-c5` 是 NimBLE，不是 Bluedroid。**
   原 header 理由「legacy 用 Bluedroid 所以 transport 必须 Bluedroid」**是错的**：
   legacy 用的是 `Arduino BLE` 这个**库**，库在 3.x 里两套 host 都支持；而 C5 的 lib
   包本身就是 NimBLE 构建的，**没有 Bluedroid 可选项**。判定 BT host 只能读 `sdkconfig.h`。
2. **地址字节序是唯一「编得过但必错」的地方。**
   NimBLE 的 `BLEAddress::getNative()` 是 **LSB first**，seam 要打印序 ⟹
   `scanResults()` 必须反转。直拷 `memcpy` 会把每个地址六个字节全交换。
   （`connect()` 方向两个 host 都吃打印序，不用动。）
3. **宏参数名撞结构体成员名**：`#define HH_UUID_LEN(u) ((u).u.type)` 会把成员名
   `.u` 一起替换掉。参数改名即可。

**验证（真 toolchain，非推断）**

| 目标 | 命令 | 结果 |
|---|---|---|
| `nm-cyd-c5`（C5 / 3.3.6 / **NimBLE**） | `syntax_env_check.py --env nm-cyd-c5` | **13/13 PASS** |
| `esp32-cyd`（ESP32 / 2.0.17 / **Bluedroid**） | `syntax_env_check.py --env esp32-cyd` | **13/13 PASS** |
| 宿主 `!ARDUINO` 分支 | `ziglang c++ -std=gnu++17 -Iinclude -c` | PASS |
| `--baseline`（未改动文件） | 两个 env | PASS（`esp32-cyd` 本轮从「必 FAIL」修成 PASS） |

**仍未验证 / 未做（不要读成「已可用」）**

- **没有任何人 `new Esp32BleCentral`。** 传输层就位但运行时依旧不可达 —— 这是 `U2` 的工作。
- **未链接、未上机**：`-fsyntax-only` 只证编译。地址反转、NimBLE live 扫描
  （`wantDuplicates=true` 才会逐条实时回调，否则只在 `DISC_COMPLETE` 补发）两条结论
  需要真机复验 ⟹ 记入 `X2`。
- **`esp32-cyd` 分支只是「能编过」**：`writeValue()` 在 2.0.17 返回 `void`，
  实现里把它如实标为「链路活着即 Ok」，不是 NotPermitted —— 语义上弱于 C5 路径。

---

## 0.7 `U2` 落地：把架构接通（2026-10-16）

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

## 0.8 外部证据复核（2026-10-16）：说明书 / 许可 / 社区反馈

> 完整取证见 **`docs/upstream-analysis/USER_EVIDENCE_zh.md`**（来源分级 S1/S2/S3、逐条引用、复现命令）。
> 本节只列**改变了本清单判断**的部分，不重复取证内容。

| # | 事实 | 证据（级） | 对清单的影响 |
|---|---|---|---|
| **E1** | 上游 v4.0 的许可是**专有**文本：禁止反向工程、**由二进制衍生**、再分发、商业分发、镜像、**改动 branding/attribution 声明**、**声称作者**；并且**源码与 `.bin` 都不存在**（分发明面只有 `flash.halehound.com`） | 上游 `LICENSE` 全文（S1） | 新增 §8 合规轨道；本 fork「按 release notes 独立实现」的路线得到正面确认，但**对外物料出现硬约束** |
| **E2** | **本 fork 自己的 `LICENSE` 是 MIT**（双著作权：`2026 Jesse C. Hale (HaleHound-CYD Edition)` + `2023 CiferTech (original ESP32-DIV)`）——即上游 v3.4.0 时代的文本 | 本仓 `LICENSE`（S1） | MIT 授权对已获得的版本不可撤销，现状合法；但**不能把 v4.0 的新材料当 MIT 用** |
| **E3** | 本 fork 链接 **`RF24@1.4.11` = GPL-2.0-only** 与 **`arduinoFFT@2.0.4` = GPL-3.0** —— 两者**互不相容**，且都不能进入专有分发 | 包内 `library.json` / `LICENSE`（S1，本地可复跑） | **在当前依赖集下固件二进制不可分发** → `C1`。上游正是靠自写 `nrf24_raw.h` / `halehound_fft.h` 绕开同一问题，并在 LICENSE 里当作卖点 |
| **E4** | 上游 **PN532 走 SPI**（`SS=GPIO17`，DIP 设 SPI；社区手册同表：MOSI/MISO/SCK = IO23/19/18），而本 fork 的 RFID 栈**只有 I2C**（`WirePn532Link`：`Wire.begin` + 100 kHz + ready byte `0x01`，PN532 相关文件 SPI 计数为 0） | 上游 README + 社区手册 + 本仓源码（S1） | `H2` 从「待决策」升级为**接线分歧**；与 `U4`（孤儿 `Pn532I2cDriver`）合并成**一次**决策 |
| **E5** | 上游讨论最热的故障是**共享 VSPI 下 SD 卡识别失败**（#165 九条评论 / #168 七条）与 **CC1101 冻结**（#172，**十四条，最多**）；另有 PSRAM 初始化失败 + Tools/Skimmer 冻结且**串口无输出**（#183）、开机 brownout 重启循环（#182） | 上游 issue 列表（S1） | 新增 `H1`（总线仲裁，**P0**）；`F1/F2/F3` 优先级上调；`F7` 的「低内存优雅退出」需要把「无日志」也算作失败模式 |
| **E6** | 本 fork 的蓝牙菜单**已经有** `Back to Main Menu`（上游 #187 恰好缺这个），但行数 10→15 使竖屏行高 25→16 px | 本仓 `LegacyApp.inc:441` + `§0.7` 实测 | `D7` 分页从「体验优化」升为 **P1**：**这是本 fork 相对上游唯一变差的地方** |
| **E7** | `A10` 里那 16 条空壳中的 **14 条**是上游菜单里**真实存在**的 Settings(9) + Tools(5) | 上游 README 菜单树（S1） | `U5` 的口径改为「补上游已有功能」，不是「清理多余条目」 |

**一句结论**：这轮外部证据**没有推翻任何已完成的工作**，但新增了一条会决定「能不能发货」的轨道（合规），并把三件原本排在后面的系统性问题（共享总线、PN532 总线、菜单分页）提到 P0/P1。

---

## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）

| ID | 事项 | 为什么阻塞 | 需要 BOSS 做什么 |
|---|---|---|---|
| **X1** | 沙箱外跑一次 `pio run` | 本沙箱守卫「每轮 50 文件删除预算」被 PlatformIO 包管理器结构性撞穿，`pio run` 在产出 `.bin` 前必失败（见 `DEVELOPMENT_PLAN.md` §5.4） | 在沙箱外执行 `pio run -e nm-cyd-c5 -e esp32-cyd`，或批准一次性提升删除预算。**本轮 7 个新 TU 已用真实 C5 工具链 `-fsyntax-only` 验证 7/7 通过（不链接、不跑 `extra_scripts`）** |
| **X2** | 两台实机上机跑 B1–B4 | `HeapBaseline` / `FeatureSwitchLedger` 只有模型自测数据，没有真实采样。**宿主侧桥接已就位**（`BleVisitAudit`：把 `IBleCentral` 的实时资源读数与 heap 采样送进两个模型），缺的只是板子上的读数 | 提供 nm-cyd-c5 + esp32-cyd 实机各一台 + SD 卡 + 拨码可切换 |
| **X3** | HID reader 服务 UUID 证据 | `0xFD40`(Signo) / `0xFFF0`(iCLASS SE) 映射无权威来源；`0xFFF0` 在 SIG 清单里是 Triones/ELK-BLEDOM 厂商服务，跨语境撞号 | 提供设备实测广告包、或厂商文档、或明确接受「凭经验映射」并注明来源等级 |
| **X4** | 魔术卡后门证据 | 写 UID 后门命令未公开验证，已按证据门标 `UNSUPPORTED`（`docs/protocol-evidence/mifare-classic.md` §4） | 提供可复现的魔术卡型号 + 后门流程，否则保持 `UNSUPPORTED` |
| **X5** | 其它密钥表来源 | KeeLoq 厂商密钥、iCLASS Corporate 1000 主密钥表 | 提供来源或明确不做 |
| **X6** | 依赖许可合规（`C1`） | **`RF24` 是 GPL-2.0-only，`arduinoFFT` 是 GPL-3.0，二者互不相容**；本 fork 的 MIT 声明覆盖不了这个组合 ⟹ **分发二进制在许可上不成立** | 三选一：① 用自研/permissive 实现替换这两个库（照上游 clean-room 做法）② 接受整个固件按 GPL 分发并履行源码义务 ③ 明确永不分发二进制（仅自用） |
| **X7** | 商标与命名边界（`C3`） | `HaleHound™` 是上游商标，且上游许可禁止「声称作者」；本 fork 若要商品化（NM 系列 / WORKBUDDY 品牌），命名与物料必须划线 | 定稿：哪些位置保留**归属性**引用（说明来源），哪些位置必须改用本产品名 |
| **X8** | PN532 总线决策（`H2`） | 上游/社区标准是 **SPI**，本 fork 只有 **I2C** ⟹ 本 fork 目前驱动不了社区那套接线；同时仓里还有一个孤儿 `Pn532I2cDriver` | 决策：① 本 fork 板另设计（I2C）→ 文档必须显式写出与上游接线的差异 ② 兼容社区接线（SPI）→ 补 SPI 链路（PN532 的 SPI 帧格式与 I2C 不同，不是换个 `Wire` 就行） |
| **X9** | 上游 4 板型 vs 本 fork 2 环境（`H4`） | 上游 4 个构建目标；本 fork 只有 `esp32-cyd` / `nm-cyd-c5`，且 `nm-cyd-c5` **不是上游任何一板**（`ST7789_DRIVER=1`、`PART_16MB=1`、`BOARD_HAS_PSRAM=1`），而上游把 ST7789 归为「另一类 CYD」 | 明确目标板清单：只支持 nm-cyd-c5 + esp32-cyd（其余不做，并在文档写明），或补齐上游板型 |

---

## 2. RFID 主战场（`DEVELOPMENT_PLAN.md` 阶段 7）

上游 v4.0 的绝对重心。**模型层已完成，PN532 驱动层本轮已打通**；剩下的是 UI、落盘与
ISO-DEP。

### 2.0 本轮落地的驱动分层（PN532）

四层，逻辑与硬件完全分离，每层一套独立宿主套件：

```
MifareClassicDriver   会话：激活 / 逐扇区认证 / 转储 / 写前拒绝   ← test_mifare_classic_driver
        ↓ 依赖
Iso14443aFraming      ISO14443-A 字节层：级联 SELECT / CRC_A / NTAG 命令  ← test_iso14443a_framing
        ↓ 依赖
IRfidTransport        传输抽象（wire seam）+ CRC_A 原语
        ↙                                    ↘
Pn532Transport        PN532 I2C 时序 + 状态码诚实映射   ← test_pn532_transport
St25r3916Transport    预留；恒 Unsupported            （无可测行为，故无套件）
        ↓ 依赖
Pn532Protocol         主机帧编解码 + Table 13 状态码     ← test_pn532_protocol
```

三条贯穿全层的设计约束（均已被测试钉死）：

1. **卡判决 ≠ 总线故障。** 状态码 `0x14` → `AuthFailed`（卡拒绝了这把密钥），未定义码 →
   `IoError`。两者若塌成一个返回值，Key Check 会把死掉的总线报成一张在保护密钥的卡。
2. **MIFARE 的 UID quirk 被关在传输层。** 认证帧末尾追加 PICC UID 这一偏离只发生在
   `Pn532Transport::transceive()` 内；`buildAuthenticate()` 保持数据手册的 8 字节形式，
   因此将来接入裸收发器（ST25R3916）时送出的仍是规范帧。
3. **「拒绝」与「发了但失败」必须可区分。** 写路径上的每一条拒绝（块 0 / 尾块 / 访问位
   不允许 / 扇区永久锁死）都发生在**任何帧上行之前**，
   `FakeRfidTransport::requestCount()` 让这一点成为可断言的属性。

> **本轮实测发现并修复的一个真 bug**：`Pn532Transport::transceive()` 在调用
> `dataExchange()` 前把 `rxLength` 清零，而该参数是**调用方容量**的入参 —— 于是任何带载荷
> 的应答都被判成「装不下」而返回 `Unsupported` 并把长度归零。只回状态字节的路径不受影响，
> 所以先前的「全绿」掩盖了它。修复后由
> `test_a_reply_too_large_for_the_caller_buffer_is_refused` 钉住契约的另一半。

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **R1** | ST25R3916 SPI 驱动 | **预留接口已就位**：`St25r3916Transport`（全方法 `Unsupported`、`caps()` 全否、`begin()` 恒 `false`）+ 10 步上电计划表 `st25r3916BringUpSteps()` + `st25r3916SiliconCaps()`。**寄存器驱动未写**（按 BOSS 指令） | 寄存器层十步：SPI 访问自检（读 `0x3F`）→ 直接命令 → IRQ → 场开关 → 软件成帧 → CRC/奇偶 → 带超时收发 → nonce → iCLASS → 仿真；板级 CS/IRQ/电源引脚需原理图确认 | 硬件 + 数据手册 | **P1**（降级：P0 项已由 PN532 承载） |
| **R2** | RFID 七屏 UI 重构 | `RfidBackendModel` 有屏幕枚举、可用性矩阵、`screenLabel()` 已修重名 | 七屏渲染 + 导航接线；按能力矩阵显隐屏项 | W1（注册） | **P0** |
| **R3** | ISO14443-A 认证链 | **已完成**（PN532 通道）：`Iso14443aFraming`（REQA/WUPA、级联 SELECT、CRC_A）+ `Pn532Transport`（`InListPassiveTarget` 在硅片内完成整个 ISO14443-3，认证帧带 UID）+ `MifareClassicDriver::authenticate()`；4 套宿主测试覆盖 | 裸收发器路径仍需 R1 的第 5/6 步（软件成帧 + CRC/奇偶） | — | ✅ |
| **R4** | MIFARE Classic 全卡转储 | **驱动层已完成**：`dumpCard()` 逐扇区走 `authenticateSectorWithTable()`，失败块记 **unknown 而非零**（有测试钉死「密钥开了但块读不出 → `recoveredSectors=1`，块全 unknown」）；转储装配/权限解码/诚实计数沿用 `MifareClassicModel` | **落盘**与进度回显，见 `R10` / `R2` | R10 | **P0**（仅剩落盘） |
| **R5** | 密钥写入 | **驱动层已完成**：`writeTrailer()` 写前检查「永久锁死（C1C2C3=111）」与「访问位写权限」，拒绝不一致的待写入访问字节，写后回读校验（Key A 读不回 → 只校验访问字节 + 可读时的 Key B）；`writeBlock()` 拒绝块 0 与尾块 | **永久报废二次确认 UI** + 与 `R2` 接线 | R2 | **P0**（仅剩 UI） |
| **R6** | 魔术卡写入 | `magicWriteAllowed(backdoor, confirmed)` 门禁就位 | 后门检测证据（`X4`）+ 驱动写入 | X4 | P1 |
| **R7** | Key Check（29 公开密钥扫描） | **驱动原语已完成**：`authenticateSectorWithTable()` 每扇区先试 NFC Forum 键（`KeyCheckSession::preferredKeyIndex`）再走 29 键表；卡离场时**立即返回 `NoTarget`/`Timeout`，而不是报「没有公开密钥能打开」**（有测试钉死） | 扫描循环的**屏与进度回显**；嵌套/硬嵌套攻击（当前**只做诚实报告，不做攻击**，需单独决策；且 PN532 根本不提供 nonce） | R2 | **P0**（仅剩 UI） |
| **R8** | NTAG Tools 写入侧 | `NtagModel`（容量/锁位/密码）+ `NdefCodec` 就位 | 擦除 / 永久只读 / 设密码 / 清密码的 **PN532 写入路径** + 危险操作确认 | PN532 驱动（本轮已就位） | P1 |
| **R9** | ISO-DEP / Type 4 全链 | 无 | I-Block 分块与 CRC、RATS/ATS、APDU 封装、NDEF TLV 映射（TLV 层可复用 `NdefCodec`） | R1 | P1 |
| **R10** | RFID dump → Loot + 机内查看 | `CardDumpFormat` 双向（`.nfc`/`.eml`，含 `??` 占位）就位 | `StorageService` 落盘 + Loot 文本查看器（分页/滚动）+ 空目录 B1 回归 | R4（驱动层已就位）+ `X2` | P1 |
| **R11** | iCLASS 射频 + 3DES | `IclassPacs` 解码就位（26/35/48-bit + Corporate 1000） | 射频层 + 3DES 密钥路径。**PN532 硅片做不到 iCLASS，只有 ST25R3916 能承载** | R1 + `X5` | P1 |

---

## 3. BLE 收口（`DEVELOPMENT_PLAN.md` 阶段 6）

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **B1** | SkeletonKey BLE 传输层 | ✅ **client 已就位**：`SkeletonKeyClient`（连接/发现/读写/notify）+ `GattTreeView`（扁平行列表、展开折叠、分页且光标跟随页）；`SkeletonKeyModel` + `GattNameTable`（165 项，对齐 SIG 机读清单）在其下 | 屏接线（`FeatureCatalog` 注册，见 `W1`）+ 真机上机（`X2`） | W1 + X2 | P1 |
| **B2** | Chameleon Ultra BLE 传输 | ✅ **client 已就位**：`ChameleonFrame`（帧编解码 + 逐字节增量接收器）+ `ChameleonUltraClient`（8 槽读写、自动连接、设备地址解析、昵称、启用槽）；`ChameleonUltraModel` 在其下。协议帧/命令号/槽类型/NUS UUID 全部逐条对过固件源码，记入 `docs/protocol-evidence/chameleon-ultra.md` | STATUS 码表（只知 `0x0000`）+ 真机上机（`X2`） | X2 | P1 |
| **B3** | HID reader tooling | ✅ **client 已就位**：`HidReaderClient`（被动扫描 + 连接诊断 + 分连接批次的扫描 + 诚实事务报告）；写目标按 GATT 属性位选，不假设回复特征。UUID 映射仍 `mappingVerified=false`，`0xFFF0` 另标 `uuidCollidesWithAnotherRole`（SIG 清单里它是 Triones/ELK-BLEDOM LED 服务） | `X3` 证据 + 屏接线；opcode 由调用方提供（本仓库不发明协议） | X3 + W1 | P1 |
| **B4** | BLE 名称库单一化 | `GattNameTable`（SIG 原文，可宿主测试）与 `include/ble_database.h`（Nordic 来源，名称截断到 14 字符，依赖 `Arduino.h`，只被 `src/legacy/bluetooth_attacks.cpp` 用）**并存** | 决定是否让 legacy 表委托新表（独立迁移，未盲改） | **BOSS 决策** | P1 |
| **B5** | BLE 内存/白屏收口 | ✅ **宿主侧已收口**：`BleVisitAudit` 把 `IBleCentral` 的实时资源读数（running/connected/scanning/subscriptions + begin/end 配对）桥进 `FeatureProfile.resourcesHeldAfterEnd`，`profileFromMeasurement()` 让「声明的 0」被实测取代，`assessBleVisit()` 复用 `HeapBaselineCheck` 的口径；三个 client 的 `end()` 是否真的交还控制器/订阅由测试实测。`FeatureSwitchLedger` + `test_ble_switch_ledger` 全绿，`replay()` 对 120 种排列 `Clean` | **真机** B2 用例 + heap 采样（`IBleHeapProbe` 的设备实现只差一个 `ESP.getFreeHeap()`） | `X2` | **P0** |

---

## 4. 台架回归（`DEVELOPMENT_PLAN.md` 阶段 6 / §5.3）

| ID | 用例 | 现状 | 通过标准 |
|---|---|---|---|
| **T1** | B1 射频后 Loot 读卡 | 模型就位 | 读到卡内既有文件，**不得空目录**；出现 `SdCode::Remounted`/`Ready`，不得 `Busy`/`NotMounted` |
| **T2** | B2 BLE 任意顺序切换 | `replay()` 已可离线验证 120 种排列 | 屏幕始终有内容（无白屏/无残留帧）；heap 不持续下降 |
| **T3** | B3 heap 基线 | `HeapBaseline` + `test_heap_baseline` 就位 | 全部 `WithinBudget`；回退 ≥8% 记 warn，≥15% 或 free <24 KiB 记 critical |
| **T4** | B4 连续运行 30 分钟 | 无 | 无看门狗复位，关键循环无持续堆下降 |
| **T5** | 沙箱外产出 `.bin` + SHA256 + 刷写清单 | `scripts/package_firmware.ps1` 就位 | `BUILD_FRESHNESS` / `FLASH_LAYOUT` / `PACKAGE_FIRMWARE` 三项 PASS | 

---

## 5. 接线与注册（低成本、高杠杆，建议最先做）

| ID | 事项 | 说明 | 级别 |
|---|---|---|---|
| **W1** | **把 13 个模型 + 5 组驱动注册进 `FeatureCatalog`** | 模型：`NdefCodec` / `CardDumpFormat` / `IclassPacs` / `KarrModel` / `RadioTuningModel` / `SkeletonKeyModel` / `ChameleonUltraModel` / `HidReaderModel` / `RfidBackendModel` / `MifareKeys` / `MifareClassicModel` / `KeyCheckModel` / `GattNameTable`。驱动：`IRfidTransport` + `RfidTransport` / `Pn532Protocol` / `Pn532Transport` + `WirePn532Link` / `Iso14443aFraming` / `MifareClassicDriver` / `St25r3916Transport`。**无 `FeatureId`、无生命周期工厂、无菜单入口**。不接线则一切「已实现」停在测试里 | **P0** |
| **W2** | `CardDumpFormat` 接到 `StorageService` | 打通 `R10` 与 `T1`（B1 回归） | P0 |
| **W3** | `LegacyApp.inc` 分域切分（Tools / Settings 优先） | 降低后续每项改动的回归面 | P1 |
| **W4** | 版本号单一来源 | 同步固件、菜单、构建产物、README | P1 |
| **W5** | CI 构建矩阵 + 格式检查 + 固件尺寸预算 | 目前仓库无 CI | P1 |
| **W6** | 收并旧的 `Pn532I2cDriver` | `src/drivers/Pn532I2cDriver.cpp` 目前只有 `probe()`；新的 `Pn532Transport` 是更完整的实现，两者并存等于同一条 I2C 总线上两套驱动。需决定：让旧驱动委托新传输，或删除旧驱动 | P1 |

---

## 6. 其余 P1 / P2（`RELEASE_BRIEF_zh.md` §5 未覆盖部分）

| ID | 事项 | 备注 | 级别 |
|---|---|---|---|
| **F1** | SubGHz 频谱分析仪 | 33 段 + peak-hold + CAL，需 RMT/CC1101 采样 + UI | P1 |
| **F2** | SubGHz Replay 目标选择器 + RSSI 门控 | 死频段移除已在 `RadioTuningModel` 固化 `offered=false` | P1 |
| **F3** | SubGHz / NRF 调参驾驶舱接线 | `RadioTuningModel` 已就位，缺 UI 接线 | P1 |
| **F4** | 统一 Loot Manager + GPS 打标 | 大类目，含 Honeypot / FlockYou / Predator 查看器扩展 | P1 |
| **F5** | Wardriving 双核 2× + 增量内存 | 消除逐行分配 | P1 |
| **F6** | Fast Touch Engine | 触摸层改造 | P1 |
| **F7** | 内存归还 + 低内存优雅退出 | **必须由 `HeapBaseline` 给出前后百分比**，不接受「感觉省了」 | P1 |
| **F8** | KeeLoq 滚动码解码 + RF600E/FM 读取 | 需厂商密钥表（`X5`） | P1 |
| **F9** | IoT Recon（厂商指纹 + SD 流式字典 + Digest） | 大工程，含主动利用路径 | P1 |
| **F10** | Drone Toolkit 四件套 | Detect / Jelly / Deauth / RID Spoof；**P2 独立审批** | P2 |
| **F11** | Radio Test 重构 | NRF 126 信道 + CC1101 RSSI 基线 + 接线图 | P2 |
| **F12** | Flock You WiFi 模式 + OUI 扩充 | 已有 `FlockFingerprint` 基础 | P2 |
| **F13** | Skimmer Hunter 主动确认 pass | 已有 `SkimmerScore` 基础 | P2 |
| **F14** | Rd-01 24GHz 雷达 | 需新硬件 | P2 |

---

## 7. 证据门（未闭合前不得实现，见 `docs/protocol-evidence/`）

| 项 | 文档 | 当前状态 | 解锁条件 |
|---|---|---|---|
| MIFARE 魔术卡后门 | `mifare-classic.md` §4 | `UNSUPPORTED` | 提供可复现型号 + 流程 |
| PN532 / ISO14443-A 字节层 | `pn532.md` | `VERIFIED`（帧、CRC_A、状态码、卡型映射，宿主侧编解码互证） | — |
| PN532 认证帧追加 UID | `pn532.md` §4 | `PARTIALLY VERIFIED`（刻意偏离数据手册） | 实机抓一次 `InDataExchange` 认证帧确认字节形态 |
| ST25R3916 寄存器层 | `st25r3916.md` | `RESERVED`（刻意未实现） | 硬件 + 原理图，见该文档 §2 |
| HID reader 服务 UUID | 待补 | 无权威来源 | `X3` |
| Airoha RACE 载荷 | `airoha-race.md` | `UNSUPPORTED` | 提供抓包与可复现步骤 |
| Tesla Charge US/EU/BOTH | — | `Unsupported` | 提供已审阅载荷与测试向量 |
| iCLASS Corporate 1000 主密钥 | — | 无 | `X5` |
| KeeLoq 厂商密钥 | — | 无 | `X5` |

> 规则（沿用 `airoha-race.md` 的口径）：**无法核验的协议部分显式标 `UNSUPPORTED` 并写明升级条件，
> 不猜、不用占位实现冒充完成。**

---

## 8. 合规与分发（2026-10-16 新增；依据 `USER_EVIDENCE_zh.md` §4 / §5）

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **C1** | 依赖许可合规 | `nrf24/RF24@1.4.11`：`library.json` 声明 **`GPL-2.0-only`**；`kosme/arduinoFFT@2.0.4`：LICENSE 正文 **GPL v3**。两者不可组合，且都与专有分发不兼容 | 三选一（同 `X6`）：替换 / 全 GPL / 不发二进制。**替换路线需要给出替代实现与回归证据**（上游的 `nrf24_raw.h`、`halehound_fft.h` 是存在性证明，不是可搬运的代码） | BOSS | **P0** |
| **C2** | 第三方声明与衍生声明 | 仓库**无** `THIRD_PARTY_NOTICES`；README 保留了 `By [JesseCHale]` / `HaleHound-CYD by [JesseCHale]` 与 CiferTech 引脚修正说明，但未按 LICENSE 建议形式声明「衍生关系 + 改了什么」 | ① `THIRD_PARTY_NOTICES.md`：逐依赖 SPDX + 版本 + 出处（含 5 个「包内无 LICENSE 文件」的补证结果）② README 加 `Based on HaleHound-CYD by Jesse C. Hale` + 本 fork 修改摘要 | `C1` | **P0** |
| **C3** | 商标与命名边界 | 对外物料使用 `HaleHound` 字样；`HaleHound™` 为上游商标 | 定稿保留/替换清单（同 `X7`），并落到 README、文档标题、未来的产品页 | BOSS | **P0（商品化前）** |
| **C4** | v4.0 专有物料的搬运边界 | 本仓文档与上游大量同名（功能名、菜单名）；README 保留上游 ASCII 横幅 | 复核边界：功能名与菜单结构属功能性描述可保留；**逐字文案、美术资产、ASCII 横幅**不得进入对外销售物料；`README.txt` / `dist/` 内若有上游原文需标注来源与许可 | `C3` | P0 |
| **C5** | 「无源码可审计」的自我约束 | 上游无源码、无二进制；本次全部上游行为描述均为**发布方自述** | 保持 `RELEASE_BRIEF_zh.md` 的标注口径，不把自述外推为已验证事实；将来若拿到实机行为，回填为 S1 | — | P2 |

---

## 9. 硬件兼容与共享总线（2026-10-16 新增；依据 `USER_EVIDENCE_zh.md` §7）

| ID | 事项 | 现状 | 还缺什么 | 依赖 | 级别 |
|---|---|---|---|---|---|
| **H1** | 共享 VSPI **总线仲裁层** | **无**。SD + CC1101 + NRF24 + PN532 全挂 GPIO 18/19/23；上游明确声称「同一时刻只有一个设备说话，SPI manager 自动仲裁」 | 一次只允许一个持有者：CS 独占、传输互斥、SD 与外设切换时的重挂载（对应上游 #165/#168 的 SD 消失）；宿主可测（假总线 + 事件序列断言，禁止「两个 CS 同时低」） | — | **P0** |
| **H2** | PN532 总线决策（SPI vs I2C） | 只有 I2C：`WirePn532Link`（100 kHz、ready byte `0x01`）；SPI 链路不存在。注释自称「NM-RF-HAT 走线较长，400 kHz 会勉强」 | 决策并落文档（同 `X8` / `U4`）：若走 I2C，必须在文档里写出**与上游接线的差异**（否则用户照官方图接线必失败）；若走 SPI，需按 PN532 SPI 帧格式新增一条 `IRfidTransport` 实现 | `U4` | **P0** |
| **H3** | 射频引脚**可重指派** | 引脚全是编译期 `-D` 常量 | 引脚→配置项（SD/EEPROM），让「几乎免焊」的 DIY 构型复用同一固件（上游 #179 的功能请求） | `H1` | P1 |
| **H4** | 闪存 / 分区矩阵 | `nm-cyd-c5` 用 `PART_16MB=1`；上游有「16 MB CYD 装不上」（#167） | 4 MB 与 16 MB 两种分区表都要能出包并过 `T5` 的 `FLASH_LAYOUT`；把 #167 写成回归用例 | `T5` | P1 |
| **H5** | 供电巡检（brownout 对策） | 文档只有「独立 3.3 V」这条要求 | 射频初始化失败时给出**指向供电的可操作提示**（而非静默失败）；把 #182 的 brownout 重启循环纳入回归 | — | P1 |

## 10. 建议执行顺序（2026-10-16 重排）

> 旧版此节有两处编号 7、两处 8、两处 9（清单被追加过但没重排）。下面是按当前依赖关系重推的顺序，
> **不是把旧列表抄一遍**。

0. **先做合规决策，再写码**：`C3`（商标/命名）→ `X6`/`C1`（GPL 三选一）→ `C2`（第三方声明）。
   **理由**：`C1` 的结论决定「是否替换 RF24 / arduinoFFT」，而这两个库牵着 2.4GHz 与频谱仪两块的做法 —— 先写码再换库等于返工。
1. **`U3`** `ChameleonUltra` 补 `FeatureId` + descriptor（P0，最小；菜单已在读 catalog，缺它即等于它不存在）
2. **`D7`** 菜单分页/滚动（P1，但这是**本 fork 相对上游唯一变差的地方**；`MenuLayout` 与 `GattTreeView` 都有现成做法）
3. **`H1`** 共享 VSPI 仲裁层（P0 结构性：后续所有射频项都建在它上面）
4. **`U4` + `H2` + `X8`** PN532 总线决策 + 孤儿驱动处置（一次决策同时关掉三张单）
5. **`W2` + `R10`** `CardDumpFormat` → `StorageService` → Loot 查看器（打通 B1 回归 `T1`）
6. **`X1`** 沙箱外 `pio run` + `.bin`（恢复「能出固件」的能力；`H4` 的分区矩阵随后）
7. **`X2` → `T1–T4`** 上机台架回归
8. **`U5`** 16 条空壳：14 条按上游 Settings/Tools 实做，2 条（`SubGhzSavedProfiles` / `SigintWardriving`）摘掉或补实现
9. **`R2`** RFID 七屏 UI（驱动前置已就位）
10. **`F1/F2/F3`** SubGHz 三件（在 `H1` 保护下才稳；上游 #172/#181 正是缺这层）
11. **`R8`** NTAG 写入侧 → **`R9`** ISO-DEP / Type 4 → **`R1`** ST25R3916（需硬件）
12. 其余 P1（含 `D4` Loot 可删除 / `D5` GPS 冷启动提示 / `H3` 引脚可重指派 / `H4` / `H5`）→ 最后 P2
