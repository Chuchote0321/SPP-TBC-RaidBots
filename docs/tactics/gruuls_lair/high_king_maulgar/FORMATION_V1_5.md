# High King Maulgar v1.5 — Encounter Formation

## Design choice

v1.5 deliberately does **not** hard-code one external database's Gruul's Lair
coordinates.

At the first `IN_PROGRESS` update, it snapshots:
- live Maulgar/Krosh/Olm/Kiggler/Blindeye coordinates;
- current raid centroid.

The vector from the council centroid toward the raid centroid becomes the
"entrance" axis for this pull. All absolute anchors are then frozen for the
fight.

This tolerates local SPP/TBC-DB spawn-coordinate differences.

## Source strategy translated into geometry

TBC strategy sources consistently recommend:
- Maulgar separated toward the entrance / wall;
- Krosh kept away from melee and Mage-tanked at range;
- Kiggler ranged-tanked;
- Blindeye and Olm on the opposite side / together enough for rapid transition.

v1.5 converts that into:

```text
                         entrance / raid side

                   Maulgar MT anchor
                         /
                        /
          [raid ranged/healer backline]

------------------ council centroid ------------------

     Blindeye anchor             Olm anchor
          \                         /
           \                       /

     Krosh ~ initial position     Kiggler ~ initial position
     Mage at 26 yd outward       Balance at 27 yd outward
```

The exact orientation rotates automatically with the live pull.

## Movement priority

```text
1. Blindeye hard interrupt
2. Enslave Demon / Krosh Spellsteal
3. Whirlwind escape
4. dedicated tank/controller anchor
5. healer/ranged/melee formation slot
6. Normal Rotation
```

A movement correction uses `MotionMaster::MovePoint`.

Formation does not issue a new move if the bot is already within tolerance:
- tank: 2.0 yd
- melee: 1.75 yd
- ranged: 2.5 yd
- healer: 3.0 yd

This is the anti-jitter hysteresis.

## Dedicated anchors

### Maulgar

The Feral MT first establishes aggro using Normal Rotation. Once Maulgar's
current victim is that tank, the formation layer pulls the tank toward the
frozen entrance-side anchor.

This avoids moving before threat exists.

### Blindeye

Same pattern:
- Prot Warrior acquires Blindeye;
- once Blindeye's victim is that Warrior, move to the opposite-side anchor.

The existing strict interrupt chain has higher priority than movement.

### Olm / Fel lane

The Olm controller gets an 18 yd ranged slot.

The Prot Paladin has a standby anchor between the Olm/Blindeye half and the
council center. As soon as an uncontrolled Wild Fel Stalker appears, that
standby rule is bypassed and the existing parallel pickup + Enslave logic owns
the bot.

### Krosh

Mage Tank:
- 26 yd radial position from live Krosh;
- position is outward from the frozen council center;
- Spellsteal is evaluated before movement.

This intentionally prioritizes Krosh Spell Shield over perfect formation.

### Kiggler

Balance tank:
- 27 yd radial outward position.

## General raid slots

### Healers

Healers occupy a deterministic entrance-side line with lateral spread.

The current implementation uses group GUID ordinal only to choose a stable lane;
it does not need a second permanent raid roster table.

### Ranged

Ranged maintain approximately 24 yd from their current kill target on the
entrance/raid side, with deterministic lateral spread.

### Melee

Melee occupy a small rear arc:
- ~3 yd from current target;
- position based on target orientation;
- five angular lanes reduce stacking.

Prot Paladin is treated as a melee formation actor when not handling a Fel.

## Maulgar Whirlwind

For player melee:

```text
Whirlwind active
→ compute vector Maulgar -> current player
→ move radially outward to ~21 yd
→ block Normal Rotation while Whirlwind remains
→ aura ends
→ normal rear-arc formation automatically pulls melee back in
```

Controlled Wild Fel Stalkers are not PlayerBot actors, so this evacuation does
not affect them. They continue meleeing Maulgar and can be consumed by
Whirlwind as specified.

## Parameters to tune after smoke test

Do not change the architecture for these. Only tune constants:

- `MAULGAR_PULL_DISTANCE = 22`
- `BLINDEYE_PULL_DISTANCE = 17`
- `OLM_PULL_DISTANCE = 17`
- `KROSH_MAGE_RANGE = 26`
- `KIGGLER_TANK_RANGE = 27`
- `OLM_WARLOCK_RANGE = 18`
- `RANGED_TARGET_RANGE = 24`
- `WHIRLWIND_SAFE_RANGE = 18`
- `WHIRLWIND_ESCAPE_RANGE = 21`

If local pathfinding shows a wall/LOS issue, first adjust these distances before
introducing absolute coordinates.
