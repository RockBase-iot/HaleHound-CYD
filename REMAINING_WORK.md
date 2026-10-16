# 剩余开发工作清单（Remaining Work）

| 项 | 值 |
|---|---|
| 快照时间 | 2026-10-16 14:21 GMT+8 |
| 分支 | `nm-cyd-c5` |
| 快照依据 | 工作树全量宿主回归 `48/48` 套件通过（`ziglang` clang 21）+ **运行时可达性只读走查**（见 §0.5，10 条硬证据） |
| 上游基线 | v4.0（见 `docs/upstream-analysis/RELEASE_BRIEF_zh.md`） |
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
  **真·① `U2` 接线**（授权钩子 + 渲染钩子 + 菜单读 catalog；传输层已就位但**无人 new**）→
  ② `X2` 上机跑一次真实 BLE 连接（地址反转 / live 扫描两条结论待真机复验）→
  ③ `CardDumpFormat` 落 SD（打通 B1 回归）。

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
| **U2** | **把架构接通**：`AppKernel` 用 `PolicyService` 填 `FeatureRunnerHooks`；给 `IFeature` 加渲染钩子；legacy 菜单改为读 catalog | A2+A3+A6 必须一起解：不接则「授权单点」是装饰、「菜单」是硬编码、「新屏」无处画 | **P0** |
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

## 1. 阻塞项（必须先解决，否则后续大量工作无法验收）

| ID | 事项 | 为什么阻塞 | 需要 BOSS 做什么 |
|---|---|---|---|
| **X1** | 沙箱外跑一次 `pio run` | 本沙箱守卫「每轮 50 文件删除预算」被 PlatformIO 包管理器结构性撞穿，`pio run` 在产出 `.bin` 前必失败（见 `DEVELOPMENT_PLAN.md` §5.4） | 在沙箱外执行 `pio run -e nm-cyd-c5 -e esp32-cyd`，或批准一次性提升删除预算。**本轮 7 个新 TU 已用真实 C5 工具链 `-fsyntax-only` 验证 7/7 通过（不链接、不跑 `extra_scripts`）** |
| **X2** | 两台实机上机跑 B1–B4 | `HeapBaseline` / `FeatureSwitchLedger` 只有模型自测数据，没有真实采样。**宿主侧桥接已就位**（`BleVisitAudit`：把 `IBleCentral` 的实时资源读数与 heap 采样送进两个模型），缺的只是板子上的读数 | 提供 nm-cyd-c5 + esp32-cyd 实机各一台 + SD 卡 + 拨码可切换 |
| **X3** | HID reader 服务 UUID 证据 | `0xFD40`(Signo) / `0xFFF0`(iCLASS SE) 映射无权威来源；`0xFFF0` 在 SIG 清单里是 Triones/ELK-BLEDOM 厂商服务，跨语境撞号 | 提供设备实测广告包、或厂商文档、或明确接受「凭经验映射」并注明来源等级 |
| **X4** | 魔术卡后门证据 | 写 UID 后门命令未公开验证，已按证据门标 `UNSUPPORTED`（`docs/protocol-evidence/mifare-classic.md` §4） | 提供可复现的魔术卡型号 + 后门流程，否则保持 `UNSUPPORTED` |
| **X5** | 其它密钥表来源 | KeeLoq 厂商密钥、iCLASS Corporate 1000 主密钥表 | 提供来源或明确不做 |

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

## 8. 建议执行顺序

1. **W1** 新模型进 `FeatureCatalog`（低成本，让已有成果从「测试里」变成「固件里」）
2. **W2 + R10** `CardDumpFormat` → `StorageService` → Loot 查看器（打通 B1）
3. **X1** 沙箱外 `pio run` + `.bin`（恢复「能出固件」的能力）
4. **X2 → T1–T4** 上机台架回归（B1–B4）
5. ~~R3 / R4(驱动) / R5(驱动) / R7(驱动)~~ **已完成** —— PN532 通道，4 套宿主测试
6. **R2** 七屏 UI 重构（驱动前置已就位，可直接开工）
7. **R8** NTAG 写入侧（PN532 路径；`InDataExchange` 与 `NdefCodec` 均已具备）
8. **R10** dump → SD 落盘 + Loot 查看器（打通 B1）
9. **R9** ISO-DEP / Type 4（只依赖 `IRfidTransport`，可先于 R1 开工）
10. **R1** ST25R3916 寄存器驱动（需硬件；只有 iCLASS 与 nonce 捕获非它不可）
7. **B1/B2/B3** BLE 传输层
8. **R9** ISO-DEP / Type 4
9. 其余 P1 → P2
