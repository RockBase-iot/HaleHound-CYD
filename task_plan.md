# Task Plan: HaleHound-CYD 3.8.1 PlatformIO 重构需求

## Goal
形成一份以 PlatformIO 为基础、覆盖 HaleHound 3.8.1 功能、仅内建支持 NM-CYD-C5 与 ESP32-2432S028，并将 NM-RF-HAT 外设能力设计为运行时可探测可选模块的完整需求与重构方案。

## Current Phase
Phase 4

## Phases

### Phase 1: 现状与基线调查
- [x] 核对用户提供的 3.8.1 README
- [x] 盘点当前分支、PlatformIO 环境与已有迁移成果
- [x] 识别 3.8.1 功能与当前代码的差距
- **Status:** complete

### Phase 2: 边界与方案确认
- [x] 澄清 3.8.1 的代码基线来源
- [x] 比较硬件抽象与迁移策略
- [x] 确认方案 A：双主板构建、共享功能核心、运行时外设注册表
- [x] 确认功能注册、生命周期、策略守卫与导航设计
- [x] 确认存储、设置、日志、更新与故障恢复设计
- [x] 确认 README 3.8.1 逐功能矩阵与 IR/RF433 扩展边界
- [x] 确认双目标构建、自动检查、硬件诊断与 C5 SPI 压力验收设计
- [x] 分章节确认整体应用框架设计
- **Status:** complete

### Phase 3: 需求与设计文档
- [x] 编写逐功能的用户流程、协议行为、依赖、降级和验收需求
- [x] 编写板卡与外设能力模型
- [x] 编写迁移、兼容和验收标准
- [x] 单列 README 3.8.1 合同与本项目 IR/RF433 扩展需求
- [x] 完成 VALHALLA/Blue Team 安全策略修订与规格自审
- [x] 用户审阅正式设计文档
- **Status:** complete

### Phase 4: 实施计划
- [x] 将重构拆分为可交付阶段
- [x] 标明文件、构建目标与验证命令
- [x] 交由用户审阅后再进入代码实现
- **Status:** complete

## Key Questions
1. “最新版本 3.8.1”是以用户提供 README 描述为准，还是另有对应源码包/分支作为代码基线？
2. NM-RF-HAT 是否统一作为可选扩展板能力，而不再作为独立主板构建目标？
3. 红外、CC1101、nRF24L01、PN532-I2C 的探测失败是否统一保留菜单并在进入功能时提示缺失？

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| 用户确认设计前不修改功能代码 | brainstorming 工作流要求先确认架构，避免在版本基线不明时错误迁移 |
| 保留现有未提交改动 | 工作树包含用户修改，不能覆盖或混入本次需求梳理 |
| 在现有过渡骨架上渐进重构 | 已有 `src/app/LegacyApp` 与 `BoardProfile`，推倒重来会增加回归风险 |
| 以用户提供的 v3.8.1 README 为功能验收合同 | 用户确认不存在对应源码，缺失能力需在当前代码上实现并逐项验收 |
| 每个 RF/外设模块独立建模 | 用户要求 NM-RF-HAT 不成为整体软件对象，以便相同驱动适用于其他扩展硬件 |
| 执行调度采用统一工作负载契约与板级后端 | ESP32-2432S028 可按负载使用 `xTaskCreatePinnedToCore()`；NM-CYD-C5 为单核，采用合作式或非固定核心执行；两者都必须服从资源仲裁与生命周期约束 |
| C5 所有 SPI 使用统一仲裁与短事务约束 | 用户确认 C5 为单核且显示/触摸/SD/射频复用 SPI，必须从框架层消除冲突，且不得以统一 TaskScheduler 作为跨板硬性要求 |
| 采用方案 A 并进行整体应用框架调整 | 用户确认双主板构建、共享功能核心、运行时外设探测，并要求不局限于局部补丁 |
| 功能入口统一经过注册表、策略守卫和外设预检 | 用户已确认第 3 节；功能不得再从菜单处理器直接进入阻塞循环或互相调用 |
| SD 开放目录与设置菜单以 README 3.8.1 为兼容合同 | 用户已确认第 4 节；存储实现可重构，但不得改变既定目录、文件格式和九类设置行为 |
| 功能矩阵按行为证据而非源码名称验收 | 用户已确认第 5 节；现有入口仍需双板复验，IR/RF433 单列为项目扩展，不冒充 README 原生功能 |
| 构建与硬件验收以双目标和可追溯证据为准 | 用户已确认第 6 节；C5 必须建立有效构建基线，硬件诊断须经资源服务层，且 SPI 压力测试必须覆盖显示、触摸、SD 与射频争用 |
| 采用渐进迁移与分阶段发布 | 用户已确认第 7 节；每阶段保持双板可构建可启动，LegacyFeatureAdapter 仅在全部迁移和审计通过后删除 |
| 独立正式设计文档作为实施基线 | 用户要求建立最新独立文档；旧 `DEVELOPMENT_PLAN.md` 只保留为历史调查资料 |
| VALHALLA 采用默认拒绝的单一持久化策略状态 | 所有攻击性功能统一门禁；拒绝、状态缺失或损坏均进入持久化 Blue Team，只有重新阅读完整免责声明并明确接受才能解锁 |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| `python` 不在 PATH，session-catchup 无法运行 | 1 | 已检查 `py`，但系统未安装 Python；改为通过 Git 状态和现有计划文件手工恢复上下文 |
| 无法在 `C:\tmp` 创建 NM-RF-HAT 临时工作树 | 1 | 改用仓库内 `.codex_tmp` 临时目录，完成只读核对后清理 |
| 板卡分区检查命令使用了错误路径及 Windows 不支持的 `*.csv` 参数 | 1 | 使用实际路径 `boards/NM-CYD-C5/platformio.ini` 并由 PowerShell 枚举 CSV 后完成核对 |
| 缺失功能的 `rg` 零匹配退出码中断并行读取 | 2 | 不再重复该组合；缺失项改用 PowerShell `Select-String`，其余确定有匹配的检索单独执行 |
| Windows 沙箱 ACL 辅助程序中断一次并行只读检查 | 1 | 缩小为技能与仓库内读取两组后成功继续，无文件受影响 |
| 构建日志中无匹配行导致 `rg` 聚合再次返回 1 | 3 | 达到三次后停止该模式，日志统一改用 PowerShell `Select-String`/`Get-Content` |
| 系统 `python` 仍不在 PATH | 2 | 使用 Codex 工作区依赖提供的显式 Python 运行 session-catchup，恢复成功 |
| 首次 VALHALLA 规格检查因 PowerShell 字符串转义错误而失败 | 1 | 改用逐项 `Select-String` 与独立条件检查，避免嵌套引号 |
| 第二次 VALHALLA 规格检查因精确匹配表达式错误而误报 | 2 | 改为验证必要语义和禁止状态项，最终检查通过 |
| 枚举源码与测试目录时包含尚不存在的 `test/`、`tests/`，`rg --files` 返回 1 | 1 | 不再重复该组合；后续只枚举实际目录，并将创建主机测试骨架列入实施计划首批任务 |
| Windows `rg` 不接受 `include/*_attacks.h`、`include/wardriving*.h` 作为路径参数 | 1 | 停止使用路径通配；改由 PowerShell `Get-ChildItem` 枚举文件后使用 `Select-String` |
| 实施计划首次暂存检查发现 12 行 Markdown 行尾空格 | 1 | 使用 `apply_patch` 删除行尾双空格，重新暂存后再次执行 `git diff --cached --check` |

## Notes
- 当前请求首先交付需求文档与架构边界，不直接开始大规模代码重构。
- 任何代码实施都应在需求设计获用户确认后进行。

## Phase 5: C5 Boot and Landscape Artifact Regression (2026-08-03)

- [x] Reproduce the old C5 FULL-image failure at bootloader offset `0x2000`.
- [x] Package classic ESP32 bootloader at `0x1000` and ESP32-C5 bootloader at `0x2000`.
- [x] Add stale-build rejection and byte-for-byte merged-image verification.
- [x] Rebuild and package `esp32-cyd` and `nm-cyd-c5` with the responsive menu source.
- [x] Verify landscape Settings geometry is 2 columns x 5 rows and shares rectangles with touch handling.
- [x] Run target contract, architecture boundary, flash-layout, and whitespace checks.
- **Status:** complete; physical boot/display/touch confirmation remains a hardware acceptance step.
