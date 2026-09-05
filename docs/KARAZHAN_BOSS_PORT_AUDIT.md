# Karazhan boss strategy audit and phase-1 port

Status: **upstream coverage audited; Attumen, Moroes and Maiden native port
implemented; in-game tuning pending.**

## Baselines

Target development remains isolated from the repository's clean `main` branch.
This change is stacked on the completed Gruul delivery branch so shared
Strategy/Action/Trigger context wiring and the pinned TBC build gate remain
available.

Upstream encounter semantics were reviewed file by file at:

```text
repository: mod-playerbots/mod-playerbots
branch: master
commit: b949b50bfcdd4fab937781bac2d7765e39330e4b
directory: src/Ai/Raid/Kara
```

No upstream branch or directory is merged wholesale.

## Upstream boss coverage

The current upstream Karazhan Strategy registers mechanics for:

1. Attumen the Huntsman
2. Moroes
3. Maiden of Virtue
4. The Big Bad Wolf
5. Romulo and Julianne
6. The Wizard of Oz
7. The Curator
8. Terestian Illhoof
9. Shade of Aran
10. Netherspite
11. Prince Malchezaar
12. Nightbane

The upstream Strategy does not provide a Chess Event automation path.

## Existing local coverage before this phase

The local repository had only:

- Netherspite void-zone movement plus direct Aura manipulation used as a beam
  shortcut;
- Prince Malchezaar infernal proximity movement.

Those two implementations remain present temporarily but are not considered
complete ports. Netherspite's direct Aura add/remove behavior is explicitly
scheduled for replacement by real portal-to-boss beam positioning.

## Phase-1 accepted semantics

### Attumen the Huntsman

- Phase 1 is detected from Midnight plus absence of mounted Attumen.
- The dynamically selected main tank and assist tank receive separate targets.
- The assist tank moves unmounted Attumen away from the raid centroid.
- Phase 2 is detected from mounted Attumen entry `16152`.
- CMaNGOS despawns the separate Midnight actor during the mount transition, so phase 2 is keyed to entry `16152` alone rather than upstream's lingering Midnight threat-list check.
- The main tank pulls mounted Attumen to the far side of a position derived
  from Midnight's respawn location and the current raid approach direction.
- Non-healers stack behind mounted Attumen; Hunters use a larger rear distance.
- Automatic tank/DPS target-assist actions are suppressed during the encounter.
- Non-tank DPS waits five seconds after the mount transition; healing remains
  available.
- Phase-2 generic movement is suppressed while the dedicated stack Action is
  active.

### Moroes

- The six possible guests are evaluated in the same deterministic upstream
  priority order.
- One deterministic coordinator marks the first live guest with Skull.
- Non-healer, non-main-tank bots target the selected guest.
- The main tank remains on Moroes and healers retain their healing target.

### Maiden of Virtue

- The current tank pulls Maiden to a dynamic far-side slot around her respawn
  position rather than a copied room XYZ.
- During Repentance, the tank moves Maiden toward a stunned healer so the
  encounter's damage aura can break the incapacitation.
- Ranged and healers receive deterministic two-ring positions around Maiden.
- Candidate destinations use terrain-corrected Z, LOS and CMaNGOS PathFinder.
- Shamans maintain Grounding Totem and competing TBC air-totem Actions
  (Wrath/Grace/Tranquil Air, Nature Resistance and Windfury) are suppressed
  while Maiden is active.
- Generic combat-formation movement is suppressed without replacing class
  rotations.

## Positioning policy

The port retains the upstream control pattern:

```text
boss/phase Trigger
  -> encounter Action
  -> short-step Movement
  -> fine-grained Multiplier
  -> normal TBC class Strategy
```

Absolute upstream Karazhan room coordinates are not copied. Active phase-1
destinations are generated from live target positions, database respawn
positions, the raid centroid, boss orientation and stable group GUID order.

## Deferred phases

- Phase 2: Opera Event variants and The Curator.
- Phase 3: Terestian Illhoof and Shade of Aran.
- Phase 4: replace local Netherspite shortcuts and complete Prince Malchezaar.
- Phase 5: Nightbane.
- Chess Event: separate design because no current upstream boss Strategy exists.

## Validation boundary

Static contracts and compilation verify integration and API compatibility. They
do not replace a Karazhan runtime smoke test of tank selection, phase
transitions, path reachability, raid-target icons, Repentance recovery,
Grounding Totem timing and wipe/reset behavior.
