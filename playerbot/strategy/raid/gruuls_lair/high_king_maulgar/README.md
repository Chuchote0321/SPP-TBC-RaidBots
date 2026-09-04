# High King Maulgar Encounter

Status: **WIP — explicit command gate and encounter-relative automatic positioning implemented; runtime tuning pending.**

Core IDs:

- Map 565
- Maulgar 18831
- Krosh 18832
- Olm 18834
- Kiggler 18835
- Blindeye 18836
- Wild Fel Stalker 18847

## Player pull commands

The encounter does not auto-pull when the raid enters the room.

```text
/ra raid prepare maulgar
/ra raid pull maulgar
```

`prepare` derives a stable local frame from the five live council positions and
the current raid centroid. Every bot is then assigned a deterministic role slot
and moves there automatically in path-generated steps of at most 5 yards. The
implementation does not require the raid leader to place bots manually and does
not depend on a server-specific absolute XYZ table.

Generated destinations are checked against:

- terrain-corrected Z;
- line of sight from the actor;
- line of sight to the assigned council target where applicable;
- council aggro clearance during preparation;
- CMaNGOS `PathFinder` reachability.

Wait for:

```text
MAULGAR_POSITIONING_READY
MAULGAR_PULL_ARMED
```

Only then issue `raid pull maulgar`.

A bot Mage automatically Frostbolts Krosh. If the selected Mage tank is a
protected human, the Hunters remain held until that human actually engages
Krosh. A bot Warlock automatically casts Searing Pain on Olm; a protected human
Warlock receives a manual-action prompt instead.

Protected human specialists are never moved by server AI. They do not need an
exact coordinate: the Mage tank must stand 20–32 yards from Krosh with line of
sight, and the Warlock tank must stand 18–32 yards from Olm with line of sight.
Unrelated real players are not included in the automatic-position readiness
barrier.

Only a raid leader or raid assistant in the same Gruul's Lair instance can use
the two commands.

## Positioning architecture

The control structure follows `mod-playerbots/mod-playerbots`:

1. encounter state and assigned role select the positioning behavior;
2. the behavior computes or retrieves a role slot;
3. the bot advances toward that slot in short movement increments;
4. normal rotation remains blocked while mandatory positioning is incomplete;
5. emergency mechanics such as Whirlwind override ordinary movement.

The upstream Maulgar implementation uses fixed room `Position` constants for
several tanks. This port keeps the same role/action and short-step movement
semantics, but replaces those absolute constants with a frame generated from
live encounter geometry. General healers, ranged and melee are distributed by
stable group ordinal rather than per-character coordinate records.
The former `MaulgarFixedPositions` table has been removed. The legacy
`MaulgarPullCoordinator` remains only as a no-op compatibility API; it owns
no state, movement, Misdirection or opening logic.

## Locked mechanics

- Feral MT -> Maulgar.
- Prot Warrior -> Blindeye.
- Balance Druid -> Kiggler.
- Mage tank priority: Game 4504 -> Migu 4506 -> bot fallback.
- Warlock priority: Chuchote 4503 -> bot fallback.
- Three Hunter Misdirection lanes only:
  - Maulgar -> Feral MT;
  - Blindeye -> Protection Warrior;
  - Kiggler -> Balance Druid.
- Krosh and Olm never receive Misdirection:
  - Mage Frostbolt self-pull -> Krosh;
  - Warlock Searing Pain self-pull -> Olm.
- Krosh Spell Shield -> Spellsteal.
- Prot Paladin pickup and Warlock Enslave Demon run in parallel for fresh Wild
  Fel Stalkers; no Banish path.
- A Warlock already controlling or pending one Fel Stalker is excluded from
  the next Enslave assignment.
- Controlled Fel -> melee Krosh; after Krosh dies -> melee Maulgar; remains in
  Whirlwind; survivors are Uncharmed and killed after Maulgar dies.
- Blindeye interrupt chain: Warriors -> Rogue+Enhancement ->
  Mage+Elemental -> repeat.

Position source: live council geometry + raid centroid. External logs are tuning
evidence only, not an active coordinate dependency.
