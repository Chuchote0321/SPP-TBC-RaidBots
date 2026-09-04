# Gruul the Dragonkiller encounter port

Status: **native Strategy route implemented; in-game 25-player tuning pending.**

## Source baseline

The encounter semantics were reviewed file by file against:

```text
repository: mod-playerbots/mod-playerbots
branch: master
commit: b949b50bfcdd4fab937781bac2d7765e39330e4b
```

Reviewed files:

- `src/Ai/Raid/Gruul/GruulStrategy.cpp`
- `src/Ai/Raid/Gruul/GruulTriggers.cpp`
- `src/Ai/Raid/Gruul/GruulActions.cpp`
- `src/Ai/Raid/Gruul/GruulMultipliers.cpp`
- `src/Ai/Raid/Gruul/GruulHelpers.cpp`

No upstream branch or directory was merged wholesale.

## Accepted encounter semantics

- map-level Strategy activation in Gruul's Lair;
- dedicated tank and ranged positioning Triggers;
- mandatory encounter movement Actions;
- Bloodlust/Heroism suppression above 95% Gruul health;
- main-tank generic movement suppression after Gruul has acquired the tank;
- Ground Slam/Shatter movement suppression with an explicit Shatter action exception;
- short incremental movement for persistent positioning;
- normal TBC class rotations remain active whenever no mandatory movement is issued.

## Local replacements and extensions

The upstream implementation stores absolute room coordinates. This TBC port does
not copy those values. It derives a map-instance-local frame from:

- Gruul's live database respawn position;
- the current raid centroid;
- a perpendicular side axis.

The main tank is placed on the far side of the dynamic room center so Gruul faces
away from the raid. The Hurtful Strike soaker is maintained in a distinct melee
slot. Ranged and healers receive deterministic two-ring arc slots at 24-40 yards.

All persistent-position candidates use:

- terrain-corrected Z;
- actor and Gruul line of sight;
- CMaNGOS `PathFinder`;
- at most five-yard movement increments.

The local max-min Shatter planner is retained instead of upstream nearest-player
fleeing. Its candidate points now receive LOS/terrain/path validation, and the
planner reports `Handled` only when it actually issues movement. Once the bot is
at its reserved slot or becomes Stoned, only conflicting movement is suppressed;
the class engine may still choose non-movement instant casts and defensives.

## Active route

```text
DungeonStrategy
  -> enter gruul's lair
  -> GruulsLairStrategy
       -> Gruul Triggers
       -> Gruul Actions
       -> Gruul Multipliers
  -> normal Playerbot Engine arbitration
  -> existing TBC class Strategy
```

`GruulsLairTactics` continues to dispatch the specialized High King Maulgar
overlay only. It no longer calls `GruulEncounter::Update`, preventing duplicate
or pre-engine Gruul control.

## Validation boundary

The static contract and pinned CMaNGOS TBC compilation gate verify source
integration and API compatibility. They do not replace an in-game 25-player
smoke test of tank drag geometry, Hurtful Strike threat, ranged slot
reachability, Ground Slam timing, and wipe/reset behavior.
