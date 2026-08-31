# Raid compatibility layer

The files in this directory adapt WLK raid-mechanic intent to the existing TBC
CMaNGOS Playerbot runtime.

| Component | Contract |
|---|---|
| `RaidPriority` | Maps semantic raid urgency to the TBC relevance scale. |
| `RaidActionFactory` | Converts action specifications into owned `NextAction**` arrays and `TriggerNode` objects. |
| `RaidStrategy` | Routes raid hooks into `BOT_STATE_COMBAT`. |
| `RaidCoreFacade` | Wraps target selection, attack, casting, aura queries and basic runtime actions. |
| `RaidTargetResolver` | Resolves combat targets by GUID or entry; name lookup is fallback only. |
| `RaidRoleResolver` | Resolves main tank, assist tanks and encounter-specific class roles, with explicit overrides. |
| `RaidMovementAdapter` | Reuses TBC `MovementAction`, `WorldPosition`, pathing and reaction semantics. |
| `RaidObjectContexts` | Provides dedicated Strategy/Action/Trigger registration points. |

This layer must remain expansion-core-facing. Encounter directories must not
include AzerothCore headers or reproduce core adaptation code.
