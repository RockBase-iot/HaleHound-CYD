# GATT UUID 名称表 Evidence Gate

Status: `VERIFIED`（对照 Bluetooth SIG 机读清单，零例外）

Scope: `GattNameTable`（`include/features/ble/GattNameTable.h`）——UUID → 可读名称。

---

## 1. 为什么需要证据门

GATT 树如果只显示十六进制 UUID 就没法看，所以需要一张「UUID → 名称」表。
但**手工维护的名称表会漂移，而 dump 是要当证据用的**：一个错误的名称比没有名称更糟，
因为它带着肯定语气。

Bluetooth SIG 把 Assigned Numbers 以**机读 YAML** 发布，所以这张表可以被**核验**而不是被信任：

```
https://bitbucket.org/bluetooth-SIG/public/raw/HEAD/assigned_numbers/uuids/service_uuids.yaml
https://bitbucket.org/bluetooth-SIG/public/raw/HEAD/assigned_numbers/uuids/characteristic_uuids.yaml
https://bitbucket.org/bluetooth-SIG/public/raw/HEAD/assigned_numbers/uuids/descriptors.yaml
```

下载（落到 git-ignored 的 `build-reports/sig/`）：

```bash
curl -sSL -o build-reports/sig/service_uuids.yaml \
  https://bitbucket.org/bluetooth-SIG/public/raw/HEAD/assigned_numbers/uuids/service_uuids.yaml
# characteristic_uuids.yaml / descriptors.yaml 同理
```

核验：

```bash
build-reports/hostenv/Scripts/python.exe build-reports/verify_gatt_names.py
```

---

## 2. 约定（VERIFIED）

| # | 规则 | 理由 |
|---|---|---|
| 1 | SIG 已分配的 UUID，名称**逐字照抄 SIG**，包括 SIG 自己的缩写 | 这是让核验脚本**不需要任何白名单**的唯一做法 |
| 2 | SIG 未分配的 UUID，名称必须带来源标记 `(vendor)` 或 `(member)` | 社区映射不能被当成官方分配 |
| 3 | 无法核验的条目**删除**，不写「例外清单」 | 与 `mifare-classic.md` §4 同一门槛 |

**必须注意 SIG 用的是缩写**，与常见写法的差别会被误当成「表写错了」：

| UUID | SIG 官方名 | 常见写法 |
|---|---|---|
| 0x1800 | `GAP` | Generic Access |
| 0x1801 | `GATT` | Generic Attribute |
| 0x1805 | `Current Time` | Current Time Service |
| 0x180F | `Battery` | Battery Service |
| 0x1827 | `Mesh Provisioning` | Mesh Provisioning Service |

本表选择**跟随 SIG**，因为这样核验脚本可以做到零例外。

---

## 3. 首次核验查出的问题（13 处名称 + 7 处删除）

初次跑 `verify_gatt_names.py` 时表是**凭记忆写的**，结果查出一批真实错误。
这正是需要机读核验的理由：

### 3.1 名称错误（已按 SIG 修正）

`0x1800`、`0x1801`、`0x1805`、`0x1806`、`0x1807`、`0x180E`、`0x180F`、`0x1811`、
`0x1820`、`0x1825`、`0x1827`、`0x1828` 共 12 个服务名，全部由「长写法」改为 SIG 缩写。

**其中一个是真错误，不只是措辞问题：**

| UUID | 原表 | SIG | 说明 |
|---|---|---|---|
| `0x2A31` | `Scan Parameters` | `Scan Refresh` | `Scan Parameters` 是**服务** `0x1813`，不是 `0x2A31` 这个特征。属于张冠李戴 |

### 3.2 无法核验的条目（已删除）

以下 7 个 UUID **不在 SIG 机读清单里**，本仓库无法核验，因此删除而不是保留：

`0x2A1F (Temperature Celsius)`、`0x2A20 (Temperature Fahrenheit)`、
`0x2A2D (Scan Refresh)`、`0x2A2E (Boot Keyboard Output Report)`、
`0x2A2F (Boot Mouse Input Report)`、`0x2A56 (Digital)`、`0x2A58 (Analog)`

删除后这些 UUID 在屏上显示原始 UUID（约定 1：未知即空），而不是一个我们没核过的名字。
特征表条目数 `113 → 106`。

> 若要补回：先在 SIG 清单里找到归属（可能已被撤回 / 改号），再把名称逐字写进表，
> 并同步 `test_gatt_names` 里 pin 住的计数。

### 3.3 来源标记

`0xFD6F` 是 Apple/Google 的**成员分配**（Exposure Notification），不是 SIG 采纳服务，
因此标注为 `Exposure Notification Service (member)`。

---

## 4. 最终核验结果

```
verify_gatt_names: 165 table entries, 612 assigned numbers available
  assigned, name-exact : 158
  unassigned, tagged   : 7
OK: every assigned UUID matches the SIG name verbatim; every other entry declares its provenance.
```

- 服务表 44 条、特征表 106 条、描述符表 15 条。
- 表按 UUID 严格升序（`GattNameTable::tablesAreSorted()`，由 `test_gatt_names` 断言），
  因为查表是线性扫描：重复或乱序条目不会崩，只会让后面那条**永远查不到**。

---

## 5. 与既有 `include/ble_database.h` 的关系（重要）

仓库里**已有一张**服务名表：`include/ble_database.h`（来源 Nordic
`bluetooth-numbers-database` v1.0.4，56 个 16-bit 服务 + 94 个 Company ID + 52 个 Appearance）。

两者**刻意不合并**，因为用途不同：

| | `include/ble_database.h` | `include/features/ble/GattNameTable.h` |
|---|---|---|
| 名称 | **截断到 ~14 字符**（`Generic Attrib`、`Health Thermo`、`IP Support`） | SIG 原文（`GATT`、`Health Thermometer`） |
| 存储 | `PROGMEM`，依赖 `Arduino.h` | 普通 `constexpr` 数组 |
| 可测 | 否（拉不进 host 测试） | 是（`test_gatt_names`，31 个 hostcheck 套件之一） |
| 使用方 | 仅 `src/legacy/bluetooth_attacks.cpp` | 新架构 `SkeletonKeyModel` / GATT 树 / dump |

截断是**渲染层**的事，不是数据层的事，所以名称存原文、上屏时再截。
**遗留项**：`ble_database.h` 应改为委托 `GattNameTable`，而不是再存一份同样的 assigned numbers；
这属于 legacy 迁移，需要单独做，不要在盲改。

---

## 6. 未覆盖范围

| 项 | 状态 | 升格条件 |
|---|---|---|
| 真实 128-bit 厂商 UUID（非 SIG base） | 不解析，返回空 | 需要按厂商建表 + 可审计来源 |
| 32-bit 厂商 UUID | 仅当高半字为 0 才折叠为 16-bit | — |
| GAP Appearance 值 | 见 `SkeletonKey::classifyAppearance`，只映射确定项 | 对照 SIG `appearance_values.yaml` 核验 |
| Company ID | **不在本表内**，仍在 `ble_database.h` | 迁移时一并处理 |
