# High King Maulgar Encounter

Status: **WIP — source migrated from v1.6; WCL fixed-position profile pending; explicit command gate implemented.**

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

Before `prepare`, manually place the pull actors while the WCL fixed-position
profile remains disabled. `prepare` freezes the current bot positions and arms
exactly three Hunter Misdirections. Wait for:

```text
MAULGAR_PULL_ARMED
```

Only then issue `raid pull maulgar`.

A bot Mage automatically Frostbolts Krosh. If the selected Mage tank is a
protected human, the Hunters remain held until that human actually engages
Krosh. A bot Warlock automatically casts Searing Pain on Olm; a protected human
Warlock receives a manual-action prompt instead.

Only a raid leader or raid assistant in the same Gruul's Lair instance can use
the two commands.

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

Position source: WCL cohort research, not manual local-character calibration.
