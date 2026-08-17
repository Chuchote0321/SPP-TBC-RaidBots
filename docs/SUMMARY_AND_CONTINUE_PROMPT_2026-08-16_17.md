# SPP-TBC-RaidBots — 2026-08-16 今日小结 + 2026-08-17 继续工作 Prompt

---

# Part A — 2026-08-16 今日小结

## 1. 项目目标

项目目标是基于：

```text
CMaNGOS TBC 2.4.3
+
cmangos/playerbots
```

建立一个专门服务于固定 25 人 Raid 的：

```text
SPP-TBC-RaidBots
```

主要工作包括：

1. 为固定 Raid 阵容重写/优化 20 个专精 AI；
2. 去除 WotLK-only 或与 TBC Raid 目标不一致的循环；
3. 处理跨职业 Raid Buff / Debuff / Totem / Judgement / Armor duty 协同；
4. 固定 25 人 subgroup；
5. 最终完成 clean build 与游戏内 Raid 回归。

独立生产仓库目标：

```text
https://github.com/Chuchote0321/SPP-TBC-RaidBots
```

---

## 2. 当前固定开发环境

### Core

```text
C:\wow-local-servers\SPP_TBC_CompatProbe
```

### PlayerBots

```text
C:\wow-local-servers\SPP_TBC_CompatProbe\src\modules\PlayerBots
```

### Build

```text
C:\wow-local-servers\SPP_TBC_CompatProbe\build-probe
```

### CMake

```text
C:\Tools\CMake-3.29.9\bin\cmake.exe
```

### TBC define

```text
MANGOSBOT_ONE
```

### Core baseline

```text
0cd3293c75aabe1b375db0dcdeee6dedfe3cd922
```

### PlayerBots baseline

```text
3f45a8e4b0edf07cdf58a843d85368775de21c7b
```

---

# 3. 今日完成：20/20 专精 AI

## Warrior

### Protection Warrior

目标：

- Shield Block / Shield Slam / Revenge
- Devastate / Sunder
- Thunder Clap
- Demoralizing Shout
- Heroic Strike 高怒泄怒
- Taunt / emergency 保留

后续还需要全局检查：

- Rogue 5CP Expose Armor 生效后，Prot Warrior 是否仍通过 Sunder / Devastate 造成不必要 armor duty 冲突。

### Arms Warrior

目标循环：

```text
Sunder opener
Execute
Mortal Strike
Overpower
Slam
Whirlwind
Heroic Strike
```

要点：

- 不常规 Rend；
- Expose Armor 出现后停止 Sunder；
- 开场仍可承担 armor build-up。

### Fury Warrior

目标：

```text
Execute
Rampage
Bloodthirst
Whirlwind
Heroic Strike
```

不承担长期 armor duty。

---

# 4. Paladin

## Protection Paladin

目标：

```text
Holy Shield
Consecration
Judgement
Seal
Exorcism
Taunt / emergency
```

并引入：

```text
Protection Wisdom Judgement
```

用于：

```text
建立 Judgement of Wisdom
+
Wisdom 真正消失时恢复
```

## Holy Paladin

目标：

```text
Holy Light 主治疗
Flash of Light 轻伤
Emergency
Cure / dispel
```

## Retribution Paladin

重点：

```text
Crusader Strike
```

提高优先级，用于配合 Protection Paladin 的 Wisdom Judgement 续期。

后续需要实际验证 TBC Core 的：

```text
Crusader Strike refresh judgement
```

行为。

---

# 5. Hunter

## Beast Mastery Hunter

目标：

```text
Auto Shot
Steady Shot
Kill Command
Pet
```

不自动维持 Serpent Sting。

## Survival Hunter

目标：

```text
Auto Shot
Kill Command
Multi-Shot
Steady Shot
```

Expose Weakness 由暴击机制自然触发。

不让 Survival 自动跑近战位置做 trap rotation。

---

# 6. Rogue

## Combat Rogue

最关键修改：

```text
5CP Expose Armor
```

作为 Raid armor duty 高优先 finisher。

后续全局协同：

```text
Expose active
    ↓
Arms / Prot Warrior 停止无意义 Sunder maintenance
```

---

# 7. Priest

## Holy Priest

目标：

```text
Circle of Healing
Prayer of Mending
Tank Renew
exact Greater Heal Rank 1 maintenance
Shadowfiend < 50%
Cure / dispel
```

## Shadow Priest

目标：

```text
Shadowfiend < 50%
Vampiric Touch
Shadow Word: Pain
Vampiric Embrace
Devouring Plague
Mind Blast
safe Shadow Word: Death
Mind Flay filler
```

---

# 8. Shaman

## Restoration Shaman

目标：

```text
Chain Heal Rank 4
Chain Heal Rank 1
Mana Tide < 80%
Earth Shield / Water Shield
Totems
Cure
```

不让 Healing Wave / Lesser Healing Wave 成为主要 TBC Raid 循环。

## Elemental Shaman

目标：

```text
Totem of Wrath
Wrath of Air
Mana Spring
Elemental Mastery
Chain Lightning
Lightning Bolt
Purge
```

不继承不必要的自动 Healing Wave。

## Enhancement Shaman

已完成自动 Totem Twisting：

```text
Windfury Totem
    ↓
Grace of Air
    ↓
约 6.5 s
    ↓
Windfury Totem
```

并具有：

```text
低蓝 → 停止 twisting → 常驻 Windfury
```

主循环：

```text
Stormstrike
Earth Shock
Melee
Shamanistic Rage
Bloodlust / Heroism
```

TBC 不使用 Lava Lash。

---

# 9. Mage

## Arcane Mage

保留 Arcane 主循环。

Raid AOE：

```text
Blizzard
```

避免自动 Arcane Explosion。

## Fire Mage

目标：

```text
Scorch / Fireball
Frostbolt fallback
Blizzard AOE
```

不混入 WotLK：

```text
Living Bomb
```

---

# 10. Warlock

## Destruction Warlock

目标 TBC Raid 版本：

```text
Shadow Bolt filler
Backlash → Shadow Bolt
```

不把：

```text
Immolate
Incinerate
Conflagrate
```

作为常规 Shadow Destro 主循环。

后续需要三名 Destro 固定 Curse duty。

---

# 11. Druid

## Restoration Druid

目标：

```text
Tank Lifebloom 3 stack
Rejuvenation
Regrowth
Raid Lifebloom spread
Innervate < 30%
```

不以 Healing Touch 作为主循环。

## Balance Druid

目标：

```text
Faerie Fire
Insect Swarm
Moonfire
Starfire filler
Moonkin Form
Innervate < 30%
Hurricane AOE
```

TBC 不使用：

```text
Starfall
```

## Feral Druid

本项目固定作为：

```text
Bear Tank
```

不是 Cat DPS。

目标：

```text
Dire Bear Form
Growl
Feral Charge
Mangle (Bear)
Lacerate 5 stack
Lacerate <3 s refresh
Faerie Fire (Feral)
Demoralizing Roar
Maul high-rage dump
Swipe
Frenzied Regeneration
```

不常规 Challenging Roar。

不自动 Combat Enrage。

不混入 Cat rotation。

---

# 12. 最终固定 25 人 Raid subgroup

以最终截图为唯一基线。

## Group 1

```text
Fury Warrior
Beast Mastery Hunter A
Beast Mastery Hunter B
Feral Druid = Bear Tank
Enhancement Shaman A
```

## Group 2

```text
Arms Warrior
Survival Hunter
Combat Rogue
Retribution Paladin
Enhancement Shaman B
```

## Group 3

```text
Destruction Warlock A
Destruction Warlock B
Destruction Warlock C
Balance Druid
Elemental Shaman
```

## Group 4

```text
Arcane Mage
Fire Mage
Holy Priest
Shadow Priest
Restoration Shaman A
```

## Group 5

```text
Protection Warrior
Protection Paladin
Holy Paladin
Restoration Druid
Restoration Shaman B
```

---

# 13. Shaman 最终分布

这是一个非常关键的最终修正：

```text
G1 → Enhancement Shaman A
G2 → Enhancement Shaman B
G3 → Elemental Shaman
G4 → Restoration Shaman A
G5 → Restoration Shaman B
```

即：

> **5 个 subgroup 恰好各 1 个 Shaman。**

因此此前“G5 双 Restoration Shaman”的判断作废。

不需要设计：

```text
同组双 Resto Totem 仲裁
双 Mana Tide 仲裁
双 Bloodlust 仲裁
```

真正需要验证的是：

```text
每个 subgroup 的 Shaman 是否按本组职责正常工作
```

---

# 14. 固定组队工具方向

已经确定使用一个 TBC 2.4.3 客户端插件辅助固定：

```text
RaidBots25
```

核心命令：

```text
/rb25 capture
/rb25 apply
/rb25 check
```

用途：

- 第一次人工按正确 subgroup 排好 25 人；
- capture 角色名 → subgroup；
- 以后 Bot 重组后自动恢复。

只固定 Raid subgroup。

不硬锁 25 个 Bot 的绝对世界坐标。

---

# 15. Git 收尾目标

今天源码需要在：

```text
https://github.com/Chuchote0321/SPP-TBC-RaidBots
```

建立至少：

```text
archive/2026-08-16-ai20
```

以及：

```text
ai20-2026-08-16
```

tag。

如果远端 `main` 为空，则允许用同一完整 AI20 checkpoint 初始化 `main`。

---

# 16. 今日明确未完成的工作

今天不要误写成“全部项目完成”。

尚未完成的是：

```text
跨职业全局集成补丁
25 人游戏内运行回归
正式 RC
正式 v1.0.0
```

具体包括：

1. Sunder ↔ Expose Armor；
2. Protection Paladin Wisdom ↔ Retribution Crusader Strike；
3. 三 Paladin Blessing；
4. 三 Destro Warlock Curse；
5. Balance Faerie Fire ↔ Feral Faerie Fire；
6. Enhancement twisting 游戏内实测；
7. 五个 subgroup 的 Shaman 行为实测；
8. 固定 Raid subgroup 实测；
9. final clean build；
10. 25 人 Raid runtime regression。

---

# Part B — 2026-08-17 明日继续工作 Prompt

你是一名熟悉：

```text
CMaNGOS TBC 2.4.3
cmangos/playerbots
SPP Classics
TBC Raid mechanics
C++
CMake
MSVC
Git
```

的工程师。

继续 `SPP-TBC-RaidBots` 项目。

今天已经完成 **20/20 专精 AI 循环**。

明天不要重新从头设计职业循环。

直接进入：

> **全局跨职业集成审计 + Final Release Candidate 准备**

---

## B1. 固定环境

```text
Core:
C:\wow-local-servers\SPP_TBC_CompatProbe

PlayerBots:
C:\wow-local-servers\SPP_TBC_CompatProbe\src\modules\PlayerBots

Build:
C:\wow-local-servers\SPP_TBC_CompatProbe\build-probe

CMake:
C:\Tools\CMake-3.29.9\bin\cmake.exe

TBC define:
MANGOSBOT_ONE

Standalone repo:
https://github.com/Chuchote0321/SPP-TBC-RaidBots
```

---

## B2. 固定 25 人 subgroup

这是硬约束。

### G1

```text
Fury Warrior
BM Hunter A
BM Hunter B
Feral Druid = Bear Tank
Enhancement Shaman A
```

### G2

```text
Arms Warrior
Survival Hunter
Combat Rogue
Retribution Paladin
Enhancement Shaman B
```

### G3

```text
Destro Warlock A
Destro Warlock B
Destro Warlock C
Balance Druid
Elemental Shaman
```

### G4

```text
Arcane Mage
Fire Mage
Holy Priest
Shadow Priest
Restoration Shaman A
```

### G5

```text
Protection Warrior
Protection Paladin
Holy Paladin
Restoration Druid
Restoration Shaman B
```

Shaman：

```text
G1 Enh
G2 Enh
G3 Elemental
G4 Resto
G5 Resto
```

注意：

> 每组恰好一个 Shaman。

不要再设计“G5 双 Resto”的协调逻辑。

---

# B3. 工作流规则

每个集成点必须：

```text
1. 读取当前 exact source
2. 解释现有逻辑
3. 定义目标行为
4. 最小修改
5. Source Gate
6. exact-file staging
7. commit
8. Core CMake pin
9. mangosd build
10. 失败则停在本阶段
```

禁止：

```text
git add .
git reset --hard
git clean -fd
大范围整文件覆盖
未经验证 cherry-pick 历史 specialist branch
```

使用：

```text
git --no-pager diff
git diff --check
```

---

# B4. 第一项：Sunder ↔ 5CP Expose Armor

先处理这个。

目标行为：

```text
Pull
 ↓
Arms Warrior 可做 Sunder opener
 ↓
Combat Rogue 建立 5CP Expose Armor
 ↓
Expose active
 ↓
Arms Warrior 停止 Sunder
Protection Warrior 停止主动 Sunder maintenance
 ↓
Expose 消失
 ↓
合理恢复 armor fallback
```

必须检查：

```text
CastSunderArmorAction
Arms-specific Sunder action
Protection Warrior strategy
Devastate
Expose Armor trigger/action
```

重点：

> 如果 Devastate 在 TBC Core 中会自动附带 Sunder Armor，那么仅仅禁止 `CastSunderArmorAction` 不一定足够。

先读源码验证，再决定最小补丁。

不要凭记忆假设。

---

# B5. 第二项：Protection Paladin Wisdom ↔ Ret Crusader Strike

目标：

```text
Protection Paladin
  ↓
建立 Judgement of Wisdom
  ↓
Retribution Paladin Crusader Strike
  ↓
刷新 Judgement
  ↓
Protection Paladin 不重复重新建立
```

检查：

- TBC Core 的 CS 是否实际 refresh active Judgement；
- PlayerBots 的 trigger 是否只在 Wisdom 丢失后恢复；
- 是否发生 Seal / Judgement spam。

---

# B6. 第三项：三 Paladin Blessing

先审计现有：

```text
Raid blessing allocator
```

不要第一步就 hard-code。

验收目标：

```text
Tank:
NO Salvation

Physical DPS:
Kings
Might
Salvation

Caster DPS:
Kings
Wisdom
Salvation

Healer:
Kings
Wisdom
useful remaining blessing
```

只有现有 allocator 无法稳定满足时才修改。

---

# B7. 第四项：三 Destro Warlock Curse

固定职责：

```text
Destro A
→ Curse of Elements

Destro B
→ Curse of Recklessness

Destro C
→ Curse of Doom
→ fallback Curse of Agony
```

还需要考虑：

```text
某个 Warlock 死亡
某个 Warlock 不在场
某个 Warlock 无法施法
```

优先保证：

```text
Elements
Recklessness
```

不丢失。

---

# B8. 第五项：Balance FF ↔ Feral FF

目标：

```text
Faerie Fire
+
Faerie Fire (Feral)
```

共享一个 Raid armor debuff duty。

避免：

```text
两个 Druid 反复各自刷新
```

不要浪费 GCD。

---

# B9. 第六项：Enhancement Totem Twisting

固定：

```text
Enh A → G1
Enh B → G2
```

运行验证：

```text
WF
→ GoA
→ WF residual
→ ~6.5 s
→ WF
```

观察：

- 是否实际存在 residual WF buff；
- 是否发生 GCD 震荡；
- 是否因 trigger lifetime/reset 导致 timer 丢失；
- 低蓝是否停止 twisting；
- 两个 Enh 是否各自只影响本 subgroup。

---

# B10. 第七项：五组 Shaman 验证

固定：

```text
G1 Enhancement
G2 Enhancement
G3 Elemental
G4 Restoration
G5 Restoration
```

验收：

### G1

```text
WF / GoA twisting
Strength
Mana
```

### G2

```text
WF / GoA twisting
Strength
Mana
```

### G3

```text
Totem of Wrath
Wrath of Air
Mana Spring
```

### G4

```text
Restoration Shaman
caster/healer subgroup support
Mana Tide
```

### G5

```text
Restoration Shaman
tank/healer subgroup support
Mana Tide
```

因为 G4/G5 的 Resto Shaman 不在同一 subgroup：

```text
不存在同组双 Mana Tide / Totem 冲突
```

---

# B11. 第八项：固定 RaidBots25 subgroup

使用：

```text
/rb25 capture
/rb25 apply
/rb25 check
```

验证：

```text
G1..G5
```

完全匹配固定布局。

不要将 world-space formation 与 subgroup 混为一谈。

---

# B12. Final clean build

完成 integration source 后：

```powershell
& "C:\Tools\CMake-3.29.9\bin\cmake.exe" --build `
    "C:\wow-local-servers\SPP_TBC_CompatProbe\build-probe" `
    --clean-first `
    --config RelWithDebInfo `
    --target mangosd `
    -- /m:1 /v:minimal
```

记录：

```text
PlayerBots HEAD
Core HEAD
mangosd SHA256
```

---

# B13. 25 人运行回归

至少检查：

```text
1. Prot Warrior
2. Arms Warrior
3. Fury Warrior
4. Prot Paladin
5. Holy Paladin
6. Ret Paladin
7. BM Hunter
8. Survival Hunter
9. Combat Rogue
10. Holy Priest
11. Shadow Priest
12. Resto Shaman
13. Elemental Shaman
14. Enhancement Shaman
15. Arcane Mage
16. Fire Mage
17. Destro Warlock
18. Resto Druid
19. Balance Druid
20. Bear Tank
```

以及跨职业：

```text
Expose / Sunder
Wisdom / Crusader Strike
Paladin Blessings
Warlock Curses
Faerie Fire
Enhancement twisting
Shaman subgroup behavior
```

---

# B14. Release 规则

只有：

```text
Source Gate PASS
+
clean build PASS
+
25-man runtime regression PASS
```

以后才能标：

```text
v1.0.0-rc1
```

经过多次副本稳定回归后：

```text
v1.0.0
```

---

# B15. 明日开始时首先做什么

第一步：

> 检查 `SPP-TBC-RaidBots` GitHub archive checkpoint 与本地 AI20 HEAD 是否一致。

确认后立即开始：

> **Sunder ↔ Expose Armor 最终集成审计**

不要重新改 20 个职业循环。
