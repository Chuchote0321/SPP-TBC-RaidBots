# SPP-TBC-RaidBots：WLK Playerbots AI 反向移植技术路线

> 文档状态：工程基线与实施路线  
> TBC 目标仓库：`Chuchote0321/SPP-TBC-RaidBots`  
> TBC 基线提交：`4580825bef00a024c4a2978fe9b10ff9544507ad`  
> WLK 参考仓库：`mod-playerbots/mod-playerbots`  
> WLK 参考提交：`2f7d9f774987d0157c6a0d0cc08c40bec3db3945`  
> 编写日期：2026-08-31

## 1. 工程结论

WLK Playerbots AI 可以反向移植到当前 TBC Playerbot，但必须采用：

```text
保留 CMaNGOS/TBC Playerbot 运行时
+
增加 Raid 兼容层
+
按副本、按首领选择性移植 WLK 决策逻辑
```

禁止采用以下两种方式：

1. 将 WLK `src/Ai` 整目录复制到 TBC；
2. 将 WLK `src/Bot + src/Ai` 整体反向移植到 CMaNGOS。

前者无法独立编译；后者等价于在 CMaNGOS 上重新实现一套 AzerothCore
Playerbots 运行时，工作量、耦合度和回归风险都过高。

推荐实施顺序：

```text
Raid 兼容接口
→ Gruul's Lair 垂直切片
→ 其余 TBC 团本
→ 最后再选择性比较通用 AI 和职业 AI
```

## 2. 两个目录并非同一抽象层级

原始比较对象为：

```text
TBC:
playerbot/strategy/generic

WLK:
src/Ai
```

二者不应直接按目录一一比较。较合理的架构映射如下：

| TBC CMaNGOS Playerbot | WLK mod-playerbots | 作用 |
|---|---|---|
| `playerbot/strategy/Action.*`、`Strategy.*`、`Trigger.*`、`Engine.*` | `src/Bot/Engine/*` | 决策引擎 |
| `playerbot/strategy/generic` | `src/Ai/Base/Strategy` | 通用策略 |
| `playerbot/strategy/actions` | `src/Ai/Base/Actions` | 通用动作 |
| `playerbot/strategy/triggers` | `src/Ai/Base/Trigger` | 通用触发器 |
| `playerbot/strategy/values` | `src/Ai/Base/Value` | AI 状态值 |
| `playerbot/strategy/<class>` | `src/Ai/Class/*` | 职业 AI |
| 新增的 `playerbot/strategy/raid` | `src/Ai/Dungeon`、`src/Ai/Raid` | 副本和团队副本 AI |
| `playerbot/PlayerbotAI.*`、Manager | `src/Bot/*` | 生命周期、核心适配、数据与网络 |

两边仍共享 Strategy–Trigger–Action–Value 的历史架构。因此，WLK 团本代码
中的“机制意图”可以保留；需要改写的是其调用 AzerothCore API 的部分。

## 3. 可移植内容与禁止直接复制的内容

### 3.1 可以按语义移植的机制

- 首领和小怪击杀优先级；
- 主坦、副坦、法师坦、枭兽坦等职责分配；
- 打断、驱散、偷取、放逐和误导职责；
- 旋风、爆炸、碎裂等危险机制规避；
- 分散、集合、固定站位和目标距离；
- 特定阶段对普通输出、AOE、移动或嗜血的抑制；
- wipe、死亡、离队后职责重新计算。

### 3.2 必须经过兼容层改写的调用

- AzerothCore `Position`；
- `MovementPriority`；
- AzerothCore `SpellInfo`；
- `CreatureAI.h` 和 `Playerbots.h`；
- WLK/AzerothCore 的角色索引接口；
- WLK 的统一 Strategy 初始化接口；
- `std::vector<NextAction>` 值语义；
- WLK 的 Action relevance 标尺；
- AzerothCore 地图、对象和路径查询接口。

### 3.3 第一阶段必须保留的 TBC 模块

- `PlayerbotAI` 生命周期；
- `PlayerbotMgr` 和 `RandomPlayerbotMgr`；
- 当前数据库、配置和登录流程；
- 网络包处理；
- 当前 `Engine` 和 reaction engine；
- `WorldPosition`、`TravelPath` 和危险区规避；
- 已完成的 20 专精 TBC Raid 职业 AI；
- 当前旅行、任务和开放世界行为。

## 4. 关键接口差异与适配原则

### 4.1 Strategy 状态模型

TBC 按 `BotState` 拆分：

```cpp
InitCombatTriggers(...)
InitNonCombatTriggers(...)
InitDeadTriggers(...)
InitReactionTriggers(...)
```

WLK 团本策略通常使用统一入口。兼容层必须将团本逻辑严格路由到：

```text
BOT_STATE_COMBAT
```

不得让团本策略在非战斗、死亡或旅行状态下意外执行。

本仓库新增 `RaidStrategy` 基类，提供：

```cpp
InitRaidTriggers(...)
InitRaidMultipliers(...)
GetRaidDefaultActions()
```

并将它们统一映射到 TBC combat state。

### 4.2 NextAction 所有权模型

TBC 使用空指针结尾的动态数组：

```cpp
NextAction**
```

并由：

```cpp
NextAction::clone()
NextAction::merge()
NextAction::destroy()
```

管理生命周期。WLK 使用值语义容器。机械替换会造成悬空、重复释放或泄漏。

本仓库新增：

```cpp
RaidActionSpec
MakeRaidActionArray(...)
MakeRaidTriggerNode(...)
AddRaidTrigger(...)
```

由统一工厂生成可被 TBC `TriggerNode` 和 `ActionNode` 正确接管的数组。

### 4.3 动作优先级

TBC 当前标尺：

```text
ACTION_NORMAL         10
ACTION_HIGH           20
ACTION_MOVE           30
ACTION_INTERRUPT      40
ACTION_DISPEL         50
ACTION_LIGHT_HEAL     60
ACTION_MEDIUM_HEAL    70
ACTION_CRITICAL_HEAL  80
ACTION_EMERGENCY      90
ACTION_PASSTROUGH    100
```

WLK 的 `ACTION_RAID=60` 不能直接复制，因为它会与 TBC 轻治疗同级。兼容层采用
语义优先级：

| `RaidUrgency` | TBC relevance 基准 | 用途 |
|---|---:|---|
| `Routine` | 21 | 目标优先级、普通机制动作 |
| `Positioning` | 31 | 常规站位，高于普通输出 |
| `Control` | 51 | 关键控制，高于普通驱散但低于治疗 |
| `CriticalMovement` | 85 | Shatter、致命 AOE 等必须抢占的移动 |
| `Emergency` | 95 | 仅用于立即致死机制 |

所有偏移最终限制在 `ACTION_PASSTROUGH` 以下，避免越过引擎保留区。

### 4.4 移动接口

TBC 已具备：

- `MoveTo`；
- `MoveNear`；
- `Follow`；
- `ChaseTo`；
- `Flee`；
- `WorldPosition`；
- `TravelPath`；
- 危险区规避；
- reaction 中断施法和移动。

因此不替换 TBC 移动系统。本仓库新增 `RaidMovementAdapter`，供首领动作以统一
语义调用现有移动实现。

### 4.5 目标解析

首领目标不得长期依赖英文名称：

```cpp
AI_VALUE2(Unit*, "find target", "high king maulgar")
```

名称会受到本地化、大小写、数据库名称和缓存影响。目标解析顺序应为：

```text
明确 ObjectGuid
→ Creature Entry
→ 当前 combat candidate 列表
→ 名称回退（仅诊断或过渡）
```

本仓库新增 `RaidTargetResolver`，从当前目标、attackers、possible targets 等既有
Value 中按 entry 或 GUID 解析目标。名称查找只保留为明确标注的 fallback。

### 4.6 Raid 角色解析

WLK 团本代码需要比当前 `IsTank/IsHeal/IsRanged/IsMelee` 更细的职责：

- Main Tank；
- Assist Tank 1/2；
- Mage Tank；
- Moonkin Tank；
- 指定猎人、术士、治疗和 DPS。

本仓库新增 `RaidRoleResolver`：

- 支持 encounter 显式覆盖；
- 无覆盖时按现有 TBC 角色判断给出稳定 fallback；
- 主坦优先使用作为坦克的队长；
- 副坦排除主坦后按组成员顺序选择；
- 法师坦和枭兽坦提供 TBC 机制所需的专用选择器。

显式配置始终高于自动 fallback。

## 5. 目标目录

```text
playerbot/strategy/raid/
├── README.md
├── common/
│   ├── RaidActionFactory.h/.cpp
│   ├── RaidCoreFacade.h/.cpp
│   ├── RaidMovementAdapter.h/.cpp
│   ├── RaidObjectContexts.h/.cpp
│   ├── RaidPriority.h/.cpp
│   ├── RaidRoleResolver.h/.cpp
│   ├── RaidStrategy.h/.cpp
│   ├── RaidTargetResolver.h/.cpp
│   └── README.md
├── karazhan/
├── gruul/
├── magtheridon/
├── serpentshrine/
├── tempest_keep/
├── hyjal/
├── black_temple/
├── zulaman/
└── sunwell/
```

每个副本目录第一阶段只建立边界与说明，不注册伪策略。实际移植时建议使用平面
文件结构：

```text
<Instance>Strategy.h/.cpp
<Instance>Actions.h/.cpp
<Instance>Triggers.h/.cpp
<Instance>Multipliers.h/.cpp
<Instance>Helpers.h/.cpp
```

若单个副本代码量继续增长，再按首领建立子目录。

## 6. 兼容层职责

### 6.1 `RaidCoreFacade`

统一封装：

- bot、AI context 获取；
- Unit 可用性检查；
- Aura 检查；
- CanCast/Cast；
- current target 和 attack target 设置；
- 通过既有 TBC `attack` action 开始攻击；
- 中断施法；
- 停止移动；
- 调用现有命名 Action。

首领文件不得直接散落重复的核心适配代码。

### 6.2 `RaidRoleResolver`

统一封装：

- 主坦；
- 第 N 副坦；
- 法师坦；
- 枭兽坦；
- 第 N 治疗、远程、近战、猎人和术士；
- encounter 显式职责覆盖。

### 6.3 `RaidTargetResolver`

统一封装：

- GUID 解析；
- Entry 解析；
- 同地图、同实例和存活检查；
- combat candidate 汇总；
- 名称 fallback。

### 6.4 `RaidMovementAdapter`

统一封装：

- 定点移动；
- 靠近位置；
- 靠近或追击 Unit；
- Follow；
- Flee；
- reaction 语义。

### 6.5 `RaidObjectContexts`

为以下对象提供独立注册入口：

```text
RaidStrategyContext
RaidActionContext
RaidTriggerContext
```

这些上下文接入 `AiObjectContext`，但在没有实际 encounter creator 时不会启用
任何首领行为。

## 7. 副本移植顺序

推荐顺序：

```text
Gruul's Lair
→ Magtheridon's Lair
→ Karazhan
→ Zul'Aman
→ Serpentshrine Cavern
→ Tempest Keep
→ Battle for Mount Hyjal
→ Black Temple
→ Sunwell Plateau
```

排序依据：

- 角色分配复杂度；
- 环境交互数量；
- 阶段切换；
- 多目标和目标缓存；
- 路径与房间几何；
- 特殊物品或点击机制。

Sunwell 单独立项，不假定 WLK 仓库已有可直接使用的完整实现。

## 8. 第一条垂直切片：Gruul's Lair

### 8.1 High King Maulgar

应移植并验证：

- 主坦负责 Maulgar；
- 第一副坦负责 Olm；
- 第二副坦负责 Blindeye；
- 高生存法师负责 Krosh；
- 枭兽负责 Kiggler；
- DPS 击杀顺序；
- Whirlwind 规避；
- Blast Wave 安全距离；
- 法师偷取 Spell Shield；
- 术士控制 Fel Stalker；
- 猎人误导指定坦克；
- 成员死亡、掉线、离队后的职责重算；
- wipe/reset 后清除旧 GUID 和阶段状态。

### 8.2 Gruul

应移植并验证：

- 主坦位置和朝向；
- 远程常规分散；
- Ground Slam/Shatter 前强制分散；
- 紧急移动可中断施法；
- 分散结束后恢复职业循环；
- 常规定位不饿死治疗；
- 致命移动高于输出和非关键施法；
- 多次 wipe/reset 后无路径刷屏和状态卡死。

### 8.3 旧 High King 自写代码

处理原则：

1. 冻结旧实现，不继续扩展；
2. 提取其角色、目标和站位结果作为行为基线；
3. 在统一 Raid 框架中实现 WLK-derived Gruul 策略；
4. 完成 25 人回归后删除旧类、旧注册、旧配置及中间产物；
5. 生产环境只保留统一 Strategy–Trigger–Action–Multiplier 路径。

## 9. 分阶段实施

### M0：冻结来源

每个移植文件记录：

```cpp
// Ported from mod-playerbots commit 2f7d9f7...
// Original path: src/Ai/Raid/Gruul/GruulActions.cpp
// TBC adaptations: role, movement, target lookup, priorities
```

禁止持续无基线地追踪 WLK `master`。

### M1：兼容层与目录骨架

内容：

- Raid 语义优先级；
- NextAction/TriggerNode 工厂；
- combat-state `RaidStrategy`；
- 核心、移动、职责、目标适配；
- Strategy/Action/Trigger 注册入口；
- CMake 接入；
- 各 TBC 团本目录。

验收：

- 无 AzerothCore 头文件；
- 无 `MovementPriority`、AzerothCore `Position`、`SpellInfo` 等符号；
- 未注册 encounter 时现有行为不变；
- Linux/Windows/TBC clean build；
- relevance 映射不与治疗优先级冲突；
- 动态 `NextAction**` 由既有析构路径回收。

### M2：Gruul/Maulgar

只移植一个完整副本，形成编译、注册、实战、reset 的闭环。

### M3：其余 TBC 团本

每个副本独立提交，禁止一次性导入全部 `src/Ai/Raid`。

### M4：选择性比较 Base 与职业 AI

团本框架稳定后，再逐项比较：

- interrupt/dispel 目标选择；
- heal target ranking；
- tank face/rear flank；
- spread/formation；
- threat control；
- crowd control；
- consumable 使用；
- 各专精循环。

职业 AI 必须在相同装备、目标、时长和初始资源条件下做对照，不以“WLK 文件
更新”作为更优证据。

## 10. 编码约束

1. 所有首领、法术和 GameObject 使用 ID 常量，名称只用于日志。
2. encounter 状态必须能在 wipe、脱战、死亡和地图切换时重置。
3. 角色选择不得每个 AI tick 对 25 人进行无界重复计算。
4. 普通站位不得长期压制治疗。
5. 致命机制必须使用 `CriticalMovement` 或 `Emergency`，并明确允许 reaction 中断。
6. 所有 WLK 来源文件记录来源提交和改写点。
7. 不在副本文件中直接复制核心兼容代码。
8. 不引入 WLK 技能、天赋和职业资源机制。
9. 不以英文 NPC 名称作为生产目标解析主键。
10. 第一阶段不删除旧 High King 代码，待新实现完成回归后再清理。

## 11. 本次骨架提交边界

本次只完成 M1：

- 写入本技术路线；
- 创建 Raid 兼容接口；
- 接入 CMake；
- 接入 `AiObjectContext`；
- 创建九个 TBC 团本策略目录。

本次明确不包含：

- Gruul/Maulgar 实际机制；
- 任何首领 Action/Trigger/Multiplier；
- 职业 AI 替换；
- 旧 High King 实现删除；
- 数据库 schema；
- 运行时配置开关。

下一步应以 `gruul/` 为唯一目标，实现第一条可编译、可注册、可 reset、可实战
验证的 encounter 垂直切片。
