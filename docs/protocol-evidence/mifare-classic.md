# MIFARE Classic Evidence Gate

Status: `PARTIALLY VERIFIED`

Scope: `MifareKeys`（公开密钥表）与 `MifareClassicModel`（扇区几何 + 访问位）。

---

## 1. 扇区尾块（Sector Trailer）布局 — VERIFIED

16 字节尾块：

| 字节 | 内容 |
|---|---|
| 0–5 | Key A（6 字节，**永远读不出**，读命令对这段返回全零） |
| 6–9 | 访问位（4 字节） |
| 10–15 | Key B（6 字节） |

**注意：访问位是 4 个字节，不是 3 个。** `byte 9` 是保留/通用字节，出厂值 `0x69`，
不参与 C1/C2/C3 编码。这是排查访问位时的第一个坑：把 `byte 9` 当成第三个访问字节，
`FF 07 80 69` 就会解出一组自相矛盾的值。

### 1.1 位打包 — VERIFIED

`_b` 表示按位取反：

```
        bit 7   6   5   4     3   2   1   0
Byte 6: C23_b C22_b C21_b C20_b | C13_b C12_b C11_b C10_b
Byte 7: C13   C12   C11   C10   | C33_b C32_b C31_b C30_b
Byte 8: C33   C32   C31   C30   | C23   C22   C21   C20
Byte 9: （保留，出厂 0x69）
```

即三个变量各以**正、反两种形式**各存一次（结构冗余 = 可做一致性校验）：

| 变量 | 正形式 | 反形式 |
|---|---|---|
| C1 | `byte7` 高半字节 | `byte6` 低半字节 |
| C2 | `byte8` 低半字节 | `byte6` 高半字节 |
| C3 | `byte8` 高半字节 | `byte7` 低半字节 |

半字节内的 bit *i* 对应**块 *i***（i = 0..3）。

### 1.2 出厂默认 `FF 07 80 69` 的逐位演算 — VERIFIED

尾块全貌：`FF FF FF FF FF FF | FF 07 80 69 | FF FF FF FF FF FF`

```
byte6 = 0xFF = 1111 1111
  → ~C2 = 0xF  ⟹  C2 = 0x0
  → ~C1 = 0xF  ⟹  C1 = 0x0
byte7 = 0x07 = 0000 0111
  →  C1 = 0x0   ✓ 与上面一致
  → ~C3 = 0x7   ⟹  C3 = 0x8
byte8 = 0x80 = 1000 0000
  →  C3 = 0x8   ✓ 与上面一致
  →  C2 = 0x0   ✓ 与上面一致
```

⇒ `C1 = 0x0`、`C2 = 0x0`、`C3 = 0x8`。

按块取位（bit i = 块 i）：

| 块 | C1 C2 C3 | 含义 |
|---|---|---|
| 0、1、2（数据块） | `000` | 读/写/加值/减值 **全部允许 KeyA 或 KeyB** |
| 3（尾块） | `001` | KeyA 不可读、可用 KeyA 写；访问位可读可写；**KeyB 可读可写** |

三条一致性校验全部通过 —— 这组打包是唯一能满足冗余校验的解，因此可以确定。

**来源**（四个独立来源，含逐位演算示例）：
- <https://www.cupass.com/en/learn/mifare-access-bits-guide.html>（给出 byte 6–9 位图 + 出厂配置说明）
- <http://gdrfid.com/productshow.php?cid=17&id=116>（给出与上表逐位一致的位图，含 `_b` 取反标注）
- <https://www.freebuf.com/sectool/338317.html>（对 `FF078069` 的完整演算，结论 `C1xC2xC3x = 000/000/000/001`）
- <https://deepwiki.com/zhovner/mfdread/3.2-access-control-and-security>（代码级：说明访问位以正反两形式冗余存储并做一致性校验）

### 1.3 权限表 — VERIFIED

数据块（块 0/1/2）：

| C1 C2 C3 | 读 | 写 | 加值 | 减值/传送/恢复 |
|---|---|---|---|---|
| 000 | A\|B | A\|B | A\|B | A\|B |
| 010 | A\|B | never | never | never |
| 100 | A\|B | B | never | never |
| 110 | A\|B | B | B | A\|B |
| 001 | A\|B | never | never | A\|B |
| 011 | B | B | never | never |
| 101 | B | never | never | never |
| 111 | never | never | never | never |

尾块（块 3）：

| C1 C2 C3 | KeyA 读 | KeyA 写 | 访问位读 | 访问位写 | KeyB 读 | KeyB 写 |
|---|---|---|---|---|---|---|
| 000 | never | A | A | never | A | A |
| 001 | never | A | A | A | A | A |
| 010 | never | never | A | never | A | never |
| 011 | never | B | A\|B | B | never | B |
| 100 | never | B | A\|B | never | never | B |
| 101 | never | never | A\|B | B | never | never |
| 110 | never | B | A\|B | never | never | never |
| 111 | never | never | A\|B | never | never | never |

> 危险提示必须出现在 UI 上：把尾块的 C1C2C3 设成 `111` 之后该扇区**永久报废**
> （俗称「变砖」），KeyA/KeyB/访问位全部不可写。

### 1.4 回归向量 — 已 pin

首次编译试验时，`MifareClassicModel.cpp` 的尾块权限表**与上面的 §1.3 表格不一致**：
把「仅 Key A」写成了「KeyA 或 KeyB」。这是实质性错误（会让操作者以为可以用 Key B
去改写 Key A），仅靠 code review 没发现，因为语义上「差不多」。

现在由 `test/native/test_mifare_classic/test_main.cpp` 钉死，任何人改动位打包或权限表都会立刻红：

| 断言 | 值 |
|---|---|
| `decodeAccessBits(FF 07 80 69)` | `c1=0x0, c2=0x0, c3=0x8`，三条冗余校验全过 |
| 数据块 0..2 权限（`000`） | 读/写/加值/减值全部 `KeyAOrB` |
| 尾块权限（`001`） | KeyA 读 `None`；KeyA 写 / 访问位读 / 访问位写 / KeyB 读 / KeyB 写 **全部 `KeyA`**（不是 `KeyAOrB`） |
| `locksSectorPermanently`（尾块 `111`，即 `00 F0 FF 69`） | `true` |
| 不一致访问位（全 `00`、或 `FF 07 80 69` 翻一位） | `decodeAccessBits` 返回 `false`，不报任何权限 |

---

## 2. 公开密钥表 — PARTIALLY VERIFIED

**事实**：上游 `JesseCHale/HaleHound-CYD` 是零源码分发仓库（29 个 commit 全是
README/LICENSE），因此上游所谓「29 个公开密钥」的**具体清单无法核实**。

**本仓库的做法**：不猜。取公开字典 **Proxmark3（RfidResearchGroup）`mfc_default_keys.dic`**
的**文件顺序前 29 条**，逐字照抄，不重排、不去重、不增删：

- 来源：<https://github.com/RfidResearchGroup/proxmark3/blob/master/client/dictionaries/mfc_default_keys.dic>
- 选择规则：文件顺序前 29 条（该字典自带注释与优先级顺序，取前 N 条是唯一可复现、
  不需要发明排序的解释）
- 密钥值：`include/features/rfid/MifareKeys.h` 里的 `kMifareKeyTable`，每条带 `note` 字段
  记录字典原文注释

**如果 BOSS 认为上游的 29 条另有其表**：只需要替换 `kMifareKeyTable` 的内容并保持
`kMifareKeyTableCount == 29`；`static_assert` 会强制计数一致，`KeyCheckModel` 无需改动。

---

## 3. 仍未验证的部分

| 项 | 状态 | 升格条件 |
|---|---|---|
| 访问位解码的**真机行为** | 仅解码层验证 | 用一张已知访问位配置的实体卡读回尾块，比对 UI 显示 |
| 29 条密钥对**具体卡片**的命中率 | 未测 | 需要实体卡 + PN532/ST25R3916 台架记录 |
| Magic Card（gen1A）后门 | **未验证** | 不实现自动后门写入；只提供 `magicWriteAllowed()` 的**门禁策略**（必须检测到后门 + 用户确认），实际发放写入命令的部分标记 `UNSUPPORTED`，见 §4 |

## 4. Magic Card（UID 可改写卡）的证据门

「魔术卡」（gen1A / CUID）依赖**厂商未公开的后门指令序列**：用非法长度或非法 CRC
的认证帧触发「永不返回 NAK」的状态。该序列来自社区逆向，没有厂商文档，且**不同批次
的实现差异很大**。

因此本仓库：

- ✅ 实现**识别与门禁**：`MifareClassicModel::magicWriteAllowed(backdoorDetected, userConfirmed)`，
  两者缺一即拒绝，避免对真卡误发写入。
- ❌ **不实现**后门指令序列本身 —— 不写推断出来的帧。相关功能保持
  `FeatureResultCode::Unsupported`，与 `docs/protocol-evidence/airoha-race.md` 同一门槛。
- 升格条件：可审计的协议来源 + 固定测试向量 + 负向量 + 授权硬件测试记录。
