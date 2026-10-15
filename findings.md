# Findings & Decisions

## 2026-08-03 Boot and Landscape Regression Findings

- The ESP32-C5 ROM reads its second-stage bootloader at `0x2000`; classic ESP32 reads it at `0x1000`.
- The old C5 FULL image placed the bootloader at `0x1000`. Bytes at `0x2000` were `6D 6F 64 65`, which the ROM reported as little-endian `invalid header: 0x65646f6d`.
- The corrected C5 image uses `0x2000` for `bootloader.bin`, `0x8000` for `partitions.bin`, `0xe000` for `boot_app0.bin`, and `0x10000` for `firmware.bin`.
- The supplied landscape Settings photo shows the old single-column renderer. Its previous FULL image predated both `MenuLayout.cpp` and the final `LegacyApp.inc` integration.
- Current landscape Settings geometry at 320x240 with 10 items is 2 columns x 5 rows. Each rectangle is about 152x38 pixels and the final row ends at y=234, inside the display.
- Rendering and touch hit testing both use `MenuLayout::itemRect()`, so the visible two-column positions and touch targets cannot drift independently.
- Native Unity execution remains unavailable because this Windows host has no native `gcc`/`g++`; both ESP32 target compilers did compile `MenuLayout.cpp`, and both firmware targets link successfully.

## Requirements
- 基于 PlatformIO 重构整个项目。
- 功能目标与 HaleHound 最新 3.8.1 对齐。
- 主板仅支持 NM-CYD-C5 与 ESP32-2432S028（原 `esp32-cyd`）。
- `esp32-cyd-hat` 不再视为独立主板，而是 ESP32-2432S028 + NM-RF-HAT 的组合形态。
- NM-RF-HAT 同样应可与 NM-CYD-C5 组合。
- RF433/CC1101、nRF24L01、红外、PN532-I2C 均视为可选外设能力。
- 默认固件/代码框架应包含这些能力；未连接或探测失败时向用户显示“设备未找到”，而不是构建失败或隐藏整体框架。
- 当前阶段需要整理 3.8.1 需求文档，并据此调整后续代码。
- VALHALLA 是统一责任确认与安全模式：所有攻击性模块在用户明确接受免责声明前锁定；拒绝后进入并持久化 Blue Team，只允许防御性和被动模块；主屏幕必须显示 `VALHALLA` 或 `BLUE TEAM` 当前模式，并提示未经授权使用可能违反当地法律。

## Research Findings
- 当前实现已持久化 `disclaimerAccepted` 与 `blueTeamMode`，但免责声明的 DECLINE/BOOT 分支仅返回 `false`，没有设置和保存 Blue Team；与最新明确要求不一致。
- 当前多个攻击入口复制 `isOffensiveAllowed()`/`showDisclaimerScreen()` 门禁，Blue Team 拦截后仍可再次接受免责声明并解除 Blue Team；目标实现必须由 `PolicyService` 单点授权并覆盖菜单、深链和功能间交接。
- 当前 `activateValhalla()` 实际执行 SD 擦除、锁定工具、进入 Blue Team 和重启，和用户定义的“VALHALLA 责任确认协议”同名；目标设计禁止 VALHALLA 隐含删除数据，紧急清除必须使用独立名称和独立确认流程。
- 当前分支为 `nm-cyd-c5`，HEAD `4ff35a3`，最近提交说明已开始 PlatformIO 迁移。
- 工作树已有用户改动：`build-reports/esp32-e32r28t.stdout.log`、`scripts/build_matrix.ps1`，以及未跟踪的 `AGENTS.md`、`DEVELOPMENT_PLAN.md`。
- 仓库根目录同时保留旧式 `.ino/.cpp` 文件，并已有 `src/`、`include/`、`boards/`、`scripts/`，说明迁移处于中间状态。
- 本机没有可用 Python，不能运行 planning-with-files 的自动恢复脚本；已改用 Git 和文件检查。
- 用户提供的 `README.md` 明确标注 `v3.8.1`，SHA-256 为 `52AB85830DB157375D502CD0F2B3AF2673C7AD3135F2A9B2424AB3F0148A43B7`，与仓库 README 不同。
- 用户提供的 v3.8.1 README 支持表仅列 `ESP32-2432S028 / esp32-cyd`；其接线说明仍把 CC1101、NRF24、PN532 视为直接外接模块，PN532 为 SPI 模式。
- 当前 `DEVELOPMENT_PLAN.md` 已把 v3.8.1 README 定义为“目标需求而非实现证明”，但其硬件矩阵仍包含 E32R28T、E32R35T、`esp32-cyd-hat`，与用户本次要求的两主板 + 可选 NM-RF-HAT 模型不一致。
- 当前 `platformio.ini` 仍定义 `esp32-cyd-hat`、E32R28T、E32R35T 等环境，需要在获批设计中明确删除、兼容别名或降级处理。
- v3.8.1 README 的新增/显著变化包括：BLE Predator、Airoha RACE、Lunatic Fringe、WLAN Ember、SubGHz Scorch、Tesla Charge、`.Sub Read`、NTAG Tools、2.4GHz Watchdog、Full Spectrum、Flock You、Drone 四项能力、Skimmer Hunter，以及扩展的 Loot/IoT Recon。
- v3.8.1 README 实际仍列四个目标（ESP32-2432S028、E32R35T、E32R28T、`esp32-cyd-hat`）；用户本次要求明确覆盖该文档中的旧硬件模型，应以两块主板加可选 NM-RF-HAT 为新目标。
- v3.8.1 README 把 CC1101、NRF24、PN532、GPS 描述为“Required”，但用户要求外设缺失时仍可启动并提示未找到，因此新需求文档必须把“主板必需”和“功能外设可选”重新分类。
- v3.8.1 原接线为 PN532 SPI；用户指定 PN532-I2C，因此驱动接口和资源管理需把 PN532 改为 I2C 能力，并明确是否保留 SPI 兼容（待方案确认）。
- 用户还新增了红外能力；v3.8.1 README 菜单和接线中没有红外，因此它属于本项目硬件适配扩展，不是上游 3.8.1 功能复刻项。
- 现有 `DEVELOPMENT_PLAN.md` 已正确提出 `BoardProfile`、`FeatureRegistry`、资源租约、旧模块适配器与分阶段迁移；这部分可复用，但需要用新的 `HardwareProfile + PeripheralCapabilities` 模型替换“五板型”假设。
- 现有计划中的模块生命周期与资源治理方向可保留：入口轻量化、模块统一 begin/tick/end、SPI 所有权、WiFi/BLE/UART 清理、版本单一来源。
- 当前 `platformio.ini` 已改为标准 `src_dir = src`，并把 `nm-cyd-c5` 设为默认环境；`esp32-cyd` 依赖经典 `espressif32@6.9.0`，C5 环境通过 `boards/*.ini` 外部配置加载。
- `esp32-cyd-hat` 当前仅在 `esp32-cyd` 上追加 `-DNMRF_HAT=1`，说明现有模型确实把扩展板错误绑定到单一主板构建目标；新架构应将扩展板从主板环境中解耦。
- `scripts/build_matrix.ps1` 和构建日志存在用户进行中的修复，本次需求阶段不得覆盖。
- 当前 `src/` 已形成 `main.cpp + app/LegacyApp + legacy/` 的过渡结构，并存在初步 `BoardProfile`，不是完全未迁移状态；后续方案应在此基础上渐进重构，而非推倒重来。
- 当前源码已明确实现 Lunatic Fringe、2.4GHz Watchdog、Full Spectrum，NTAG 仅有基础类型识别；其余检索目标（BLE Predator、Airoha RACE、Tesla Charge、`.Sub Read`、Flock You、Drone、Skimmer、红外）未发现对应实现标识。
- C5 已在 `cyd_config.h` 中使用 PN532 I2C（SDA=9、SCL=8），经典 CYD 仍使用 PN532 SPI；这与“NM-RF-HAT 应同时适配两主板”的统一扩展板模型冲突。
- `CYD_HAS_CC1101`、`CYD_HAS_NRF24`、`CYD_HAS_PN532` 当前固定为 1，表达的是编译期假定存在，不符合“运行时未找到设备即可提示”的新要求。
- 当前 NM-RF-HAT 通过 `NMRF_HAT` 宏改变 CC1101/NRF24 引脚；需要进一步以 NM-RF-HAT 实际接口图为准定义可探测外设、总线和复用限制。
- Git 远端仅为 `RockBase-iot/HaleHound-CYD`，本地与远端引用中没有 v3.8.1 tag/branch；用户提供 README 目前是唯一可见的 3.8.1 基线资料。
- 当前 `BoardProfile` 只描述一组静态引脚和少量布尔值，仍把“板卡固有能力”和“外接设备是否存在”混在一起；目标模型应拆成静态 `BoardProfile`、扩展连接 `ExpansionProfile` 和运行时 `PeripheralRegistry`。
- `App` 目前仅转发到 `legacySetup/legacyLoop`，适合作为渐进替换缝隙；可逐步引入探测服务和功能注册表，而不立即重写全部业务模块。
- NM-RF-HAT 官方资料明确支持 ESP32-2432S028 与 NM-CYD-C5，并集成 CC1101、nRF24L01、PN532、红外 TX/RX、RF433 OOK/ASK、GNSS 与 TF 卡。
- NM-RF-HAT 的 6 位拨码中：1=CC1101、2=nRF24、3=PN532、4=IR、5=RF433、6=电池；1–5 任意时刻只能有一个置 ON，因此这些扩展功能在硬件层面互斥。
- NM-RF-HAT 用户手册指定 PN532 使用 I2C；IR 与 RF433 都复用 TX=GPIO22、RX=GPIO27（ESP32-2432S028 侧）。
- NM-RF-HAT 引脚图给出跨主板映射：经典 CYD 的 GPIO22/27 映射到 C5 GPIO8/9；经典 CYD UART GPIO3/1 映射到 C5 GPIO4/5；TF 总线 GPIO19/18/23/5 映射到 C5 GPIO2/6/7/10。
- 因拨码只连接一个功能，启动时“同时发现全部外设”在物理上不可行；运行时探测必须采用按需探测与状态缓存，未找到时同时提示检查拨码位置、供电和连接。
- 用户修正：软件模型不得包含 NM-RF-HAT 整体对象；CC1101、nRF24、PN532-I2C、IR TX/RX、RF433 TX/RX 等每个模块都是独立最小外设单元，便于同类模块通过其他接线方式扩展。
- NM-RF-HAT 仅作为已验证接线和引脚复用参考，不作为运行时设备、功能依赖或构建目标。
- 用户确认 NM-CYD-C5 为单核 CPU；现有大量 `xTaskCreatePinnedToCore(..., 0)` 与“Core 0 射频/Core 1 UI”设计必须迁移到平台无关的工作负载契约，由板级执行后端决定实际调度方式。
- NM-CYD-C5 的显示、触摸、SD 和射频模块共用 SPI2/FSPI：MISO=GPIO2、SCK=GPIO6、MOSI=GPIO7，各设备仅 CS/控制脚不同；显示渲染也必须纳入同一个 SPI 仲裁域。
- 当前 SPI manager 只使用 `currentDevice` 与 `busLocked` 普通全局变量，没有 FreeRTOS mutex、所有者、超时、RAII 或线程安全保证，不能防止任务切换造成的 SPI 冲突。
- 当前源码存在大量模块直接调用 SPI 和固定核心任务；新架构必须禁止业务层直接访问 `SPI`、TFT、SD 和射频库，并通过服务层与统一总线租约访问。
- 单核 C5 上不能依赖“射频任务与 UI 位于不同核心”获得并行性；需要合作式时间片或非固定核心任务，射频小批处理后释放 SPI，再允许触摸和显示刷新。
- 两块主板不要求统一使用 `TaskScheduler`。功能只声明工作负载角色、实时性、时间预算、栈需求和资源依赖；ESP32-2432S028 后端可使用 `xTaskCreatePinnedToCore()`，C5 后端采用单核合作式/非固定核心执行。
- 即使经典 ESP32 使用固定核心任务，也不得绕过 `ResourceBroker`、SPI 租约、退出请求和清理协议；核心隔离不能替代共享资源仲裁。
- 用户已确认第 3 节设计：`FeatureRegistry`、板级 `ExecutionPolicy`、中央 VALHALLA/Blue Team 策略、运行时外设预检、`FeatureResult` 导航和统一清理流程。
- v3.8.1 README 的设置验收项共九类：亮度 10-255、屏幕超时六档、BGR/RGB、显示反色、三种颜色模式、四向旋转、设备信息、四位 PIN、CC1101 标准/PA 模块选择。
- v3.8.1 README 要求 FAT32 MicroSD，并规定 `/subghz`、`/eapol`、`/wardriving`、`/wp_loot`、`/loot`、`/creds.txt`、`/iot_recon.txt`、`/firmware` 路径；目录或文件缺失必须作为正常空状态处理。
- v3.8.1 的 Loot 验收描述为五类统一浏览：wardriving、EAPOL、WhisperPair/BLE Predator、IoT Recon 和 credentials；Flock You 另写入 `/loot`，设计需保留该目录并允许后续注册为独立制品提供者，不能破坏五类基础验收。
- `.Sub Read` 必须支持 `/subghz` 子目录浏览，且目录缺失时显示空状态；不能把 `.sub` 文件导入私有数据库后才可使用。
- Update Firmware 从 `/firmware` 读取 `.bin`。双主板架构下必须在写入前校验 ESP32 与 ESP32-C5 镜像目标，不能只依赖文件名或用户选择。
- 当前设置采用单个 EEPROM 结构与 magic 版本，Captive Portal 和 SubGHz 等模块还各自直接使用 EEPROM；当前 SD 用户也分别执行 `SD.begin()`、`SPI.begin()` 或 `SD.end()`，与目标资源仲裁边界冲突。
- 当前 Loot Manager 固定实现五类目录/文件，适合迁移为制品提供者；存储层仍应保留 README 指定的开放文件格式和路径，不引入设备专属数据库作为唯一数据源。
- ESP32-2432S028 板定义为 4 MB Flash，NM-CYD-C5 为 16 MB；当前两个环境却都指定 `huge_app.csv`，不能直接假设具有相同 OTA 分区或回滚能力。
- 当前固件更新校验仅检查文件大小与首字节 `0xE9`，无法阻止经典 ESP32 与 ESP32-C5 镜像互刷；目标设计必须读取 ESP image header/app descriptor 并核对芯片目标、镜像长度及可写分区。
- 用户已确认第 4 节存储、设置、日志、固件更新和故障恢复设计。
- README 3.8.1 的 WiFi 八项在当前菜单和命名空间中均有对应实现；仍需在新生命周期、C5 单核策略和硬件验收下重新验证，不能仅以源码存在判定最终通过。
- README 3.8.1 的 Bluetooth 菜单与当前菜单不一致：目标需要 BLE Cinder、BLE Spoofer、BLE Beacon、BLE Predator、WhisperPair、Airoha RACE、Lunatic Fringe；当前另有 Sniffer、BLE Scanner、AirTag hub、BLE Ducky，但未发现 BLE Predator 或 Airoha RACE 实现。
- Lunatic Fringe 及其 Tracker Scan、AirTag Detect、Phantom Flood、AirTag Replay 在当前源码中均有实现；目标菜单结构需要按 README 合并到 Lunatic Fringe 层级，而不是保留平行 AirTag hub。
- README 的 NRF24 六项均有当前实现对应物，其中当前 `WLAN Jammer` 需按 3.8.1 显示名 `WLAN Ember`；功能仍依赖 nRF24 运行时探测和跨板硬件复验。
- README 的 SubGHz 目标有七项；当前只有 Replay、Brute Force、Jammer、Spectrum、Saved Profile 五项，缺 Tesla Charge 和 `.Sub Read`。当前 `SubGHz Jammer` 需按 3.8.1 命名 `SubGHz Scorch`。
- 当前 RFID 有 Card Scanner、Reader、Clone、Key Brute Force、Emulate 五项，但菜单缺 `NTAG Tools`；基础卡型识别不等于 README 要求的 NTAG213/215/216 read、dump、NDEF、write、lock、password 全套能力。
- Jam Detect 四项与 README 均有当前对应实现；Full Spectrum 与 2.4GHz Watchdog 虽存在源码，仍需验证多外设预检、资源冲突和单核 C5 时间片行为。
- SIGINT 当前有 EAPOL、Karma、Wardriving、Saved Captures、IoT Recon、Loot 六项，缺 README 的 Flock You。
- Tools 五项和 Settings 九项均有菜单入口；固件更新、Radio Test、GPS、串口冲突、触摸校准和设备信息仍需按新板卡/外设模型复验。
- README 的 Drone 四项和 Skimmer Hunter 在当前主菜单、处理器和源码中均未发现实现。
- README 明确要求 BLE Predator 三阶段：SCAN 威胁分类、RECON 全 GATT 服务/特征枚举、HONEYPOT 可连接克隆并捕获 WRITE，数据保存到 SD；当前 BLE Scanner/Sniffer 不能替代这套状态机。
- README 明确要求 Airoha RACE 覆盖 CVE-2025-20700/20701/20702，并提取 link key、BD_ADDR、固件版本与 Flash；源码零匹配，必须从协议与授权测试边界开始新实现。
- README 的 Drone Detect 明确要求 ESP32 WiFi promiscuous 与 nRF24 两条独立接收链同时工作；经典 ESP32 可双核，C5 必须用短时间片保持两路采样窗口和 UI 响应。
- README 的 Skimmer Hunter 为 BETA，但仍要求多信号评分和主动确认两阶段；“BETA”只降低现场验证成熟度，不允许用普通 BLE 扫描器占位。
- README 将 `/creds.txt` 定义为 IoT Recon 自定义凭据输入，当前代码却将其作为 Captive Portal 捕获内容并另读 `/iot_creds.txt`；目标需求必须保护用户输入并迁移捕获输出到独立 Loot 路径。
- 项目新增的 IR 与 RF433 不属于 README 3.8.1 菜单，应作为显式扩展需求单列；两者仍需独立驱动、资源路由、运行时状态和最小收发/测试能力。
- 用户已确认第 5 节逐功能状态矩阵、重点缺失功能行为和 IR/RF433 扩展边界。
- 当前 `scripts/build_matrix.ps1` 仍构建五个环境；目标验收矩阵必须只包含 `esp32-cyd` 与 `nm-cyd-c5`，同时保留 RAM、Flash 和 SHA-256 报告能力。
- 仓库没有单元测试、集成测试或构建 CI，`.github` 只有 `FUNDING.yml`；纯逻辑模块需要新增 PlatformIO native/host 测试层，硬件功能则使用可追溯的板上验收记录。
- 当前 Radio Test 只对 nRF24 和 CC1101 执行寄存器读写，GPS/PN532 页面主要是接线图；目标工具需按独立外设扩展 SD、PN532-I2C、GPS、IR 和 RF433 测试，并通过服务层而非直接重置 SPI。
- 经典 `esp32-cyd` 现有构建日志成功：RAM 121024/327680（36.9%），Flash 2359353/3145728（75.0%）。该数据只作为重构前尺寸基线，不代表 3.8.1 功能完成。
- C5 现有日志只完成 PlatformIO 平台下载解包，没有编译成功、失败或资源统计，因此 C5 尚无有效构建基线，实施第一阶段必须重新建立。
- 当前 Radio Test 直接 `SPI.end()/begin()`、手动 CS 和硬编码经典 CYD 引脚，无法作为 C5 共享 SPI 安全性的验收实现；应迁移为 `PeripheralDiagnostics` 调用统一驱动与租约。

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| 暂不把 `esp32-cyd-hat` 当作第三个主板目标 | 用户明确其本质是主板与扩展板组合，应转为能力配置 |
| 外设按运行时能力探测设计 | 同一固件可在未安装扩展板时正常启动，使用相关功能时给出明确反馈 |
| v3.8.1 README 是唯一功能验收清单 | 用户确认没有 3.8.1 源码，不能按源码移植，只能用可验证需求逐项补齐 |
| 采用方案 A | 只保留两个主板构建环境；共享业务实现；NM-RF-HAT 与外设状态在运行时管理 |
| 允许调整整体应用框架 | 用户明确要求较大范围架构调整，因此可替换主循环、导航、功能注册、资源治理和服务边界，但仍分阶段迁移旧模块 |
| 外设模块是最小硬件单元 | NM-RF-HAT 仅提供接线组合；软件分别注册 CC1101、nRF24、PN532-I2C、IR、RF433 等模块，以支持其他扩展硬件 |
| 采用板级 `ExecutionPolicy` 后端 | 共享层只定义工作负载契约；经典 ESP32 可固定核心执行，C5 使用单核合作式或非固定核心执行，不强制统一 TaskScheduler |
| C5 使用短事务和单核让出策略 | 显示、触摸、SD、射频共用 FSPI，所有工作必须可分批、可让出并经过统一总线仲裁 |
| SD 文件布局以 v3.8.1 README 为兼容合同 | 用户可以直接放入 `.sub`、`creds.txt` 和固件文件，也可以从电脑读取捕获结果；新架构只统一访问方式，不改变开放目录格式 |
| 功能完成状态必须由需求行为与验收证据判定 | 菜单字符串、命名空间或占位页面只能证明入口存在；不等于满足 README 3.8.1 的协议、存储、跨板和硬件行为 |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| 3.8.1 目前只有用户提供的 README 路径 | 用户已确认没有源码；将 README 条目转换为需求 ID、状态与验收证据 |
| 工作树非干净状态 | 只新增本次计划文件，避免修改或清理用户现有文件 |
| 分区检查首次命令路径与通配写法错误 | 改用 `boards/NM-CYD-C5/platformio.ini` 和 `Get-ChildItem -Filter '*.csv'` 完成核对，并记录该错误避免重复 |
| 两次并行 `rg` 因预期的零匹配返回码 1 而中断 | 已停止重复，缺失项采用 `Select-String` 验证为零结果，其他读取拆分执行 |
| Windows 沙箱 ACL helper 一次性中断并行读取 | 缩小读取分组后成功恢复，不需要权限升级 |
| `rg` 聚合构建日志第三次因空匹配返回 1 | 按三次错误协议永久切换为 PowerShell 日志检索，不再重试原模式 |

## Resources
- `C:\Users\RockBase-007\Downloads\README.md`
- `E:\github\HaleHound-CYD\platformio.ini`
- `E:\github\HaleHound-CYD\DEVELOPMENT_PLAN.md`
- https://github.com/RockBase-iot/NM-RF-HAT

## Visual/Browser Findings
- 本阶段主要是文本架构和需求，不需要浏览器视觉伴侣。
- NM-RF-HAT 引脚图显示同一扩展板通过不同主板接口映射到相同 HAT 功能；两块主板不能共用同一 GPIO 数值，但可共用同一逻辑信号名称。

## 详细实施计划补充调查（2026-07-28）
- `src/main.cpp` 已是薄入口，但 `src/app/App.cpp` 仍完全转发到 `legacySetup()`/`legacyLoop()`；`AppKernel` 可以在不改变 Arduino 入口的前提下逐步接管初始化和事件循环。
- `include/board/BoardProfile.h` 当前同时保存板载静态属性、CC1101/nRF24/PN532 引脚及 `radioChipSelectShared`/`pn532UsesI2c` 假设；实施时必须先拆分静态 `BoardProfile` 与独立 `ExpansionRoute`，再引入运行时 `PeripheralRegistry`。
- `platformio.ini` 当前仍包含 `esp32-cyd-hat`、`esp32-e32r28t`、`esp32-e32r35t`，`scripts/build_matrix.ps1` 也仍构建五个环境；M1 必须收敛到两个正式目标，同时合并用户当前对构建脚本的未提交修改。
- 仓库当前没有 `test/` 或 `tests/` 目录；主机纯逻辑测试需要从 PlatformIO `native` 环境和 Unity 骨架开始。
- 旧功能中存在大量直接 `SPI.begin/end`、`SD.begin/end` 和跨模块 EEPROM 访问；资源与存储服务必须先于功能类别迁移落地，并通过静态审计规则阻止新增直连访问。
- `LegacyApp.inc` 中攻击性门禁按菜单分支重复，旧 VALHALLA 状态又由两个布尔量表示；`PolicyService` 和单一持久化枚举必须在迁移任何攻击入口前完成。
