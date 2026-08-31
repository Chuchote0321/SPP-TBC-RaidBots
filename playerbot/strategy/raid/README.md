# TBC Raid strategy scaffold

This directory is the compatibility boundary for porting selected raid mechanics
from `mod-playerbots/mod-playerbots/src/Ai/Raid` into the existing
CMaNGOS/TBC Playerbot runtime.

The engineering route is documented in:

- [`docs/WLK_AI_PORTING_TECHNICAL_ROUTE.md`](../../../docs/WLK_AI_PORTING_TECHNICAL_ROUTE.md)

## Current status

The common compatibility layer is connected to CMake and `AiObjectContext`.
No encounter Strategy, Action, Trigger or Multiplier is registered yet, so this
scaffold does not enable any boss-specific behaviour by itself.

## Directory map

| Directory | Instance | Map ID | Status |
|---|---|---:|---|
| `karazhan/` | Karazhan | 532 | scaffold |
| `gruul/` | Gruul's Lair | 565 | scaffold; first implementation target |
| `magtheridon/` | Magtheridon's Lair | 544 | scaffold |
| `serpentshrine/` | Serpentshrine Cavern | 548 | scaffold |
| `tempest_keep/` | Tempest Keep: The Eye | 550 | scaffold |
| `hyjal/` | Battle for Mount Hyjal | 534 | scaffold |
| `black_temple/` | Black Temple | 564 | scaffold |
| `zulaman/` | Zul'Aman | 568 | scaffold |
| `sunwell/` | Sunwell Plateau | 580 | separate development scope |

## Expected per-instance files

Use a flat instance directory until code size justifies boss subdirectories:

```text
<Instance>Strategy.h/.cpp
<Instance>Actions.h/.cpp
<Instance>Triggers.h/.cpp
<Instance>Multipliers.h/.cpp
<Instance>Helpers.h/.cpp
```

Every ported source file must record the upstream commit, original path and
TBC-specific adaptations. NPC names are for logs only; production target
lookup uses GUID or creature entry.
