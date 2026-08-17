# Encounter v1 Architecture

## Hook

The only integration point is immediately before:

`currentEngine->DoNextAction(...)`

This is intentional. The existing PlayerBot update loop already handles:
- cast state and GCD timing
- reaction engine
- movement state
- combat/non-combat engine transitions

Encounter AI therefore does not duplicate that machinery.

## Override contract

### `NotHandled`
Either:
- no supported encounter is active; or
- the encounter made a SOFT state change such as target assignment.

Normal Rotation executes normally.

### `Handled`
A HARD mechanic action was successfully executed:
- Spellsteal
- interrupt
- flee

Normal Rotation is skipped for only that tick.

### `BlockNormal`
The mechanic demands that normal combat not run even if the hard action could
not execute in this tick. v1 uses this during Maulgar Whirlwind if melee flee
cannot start immediately.

## Actor policy

Role resolution is based on exact character low GUIDs actually present in the
group/instance.

Protected human characters are recognized as `HumanPlayer` and never moved or
cast by server Encounter AI.

## Expansion

After Maulgar acceptance:

1. Gruul
2. Magtheridon
3. SSC / Hydross + EncounterModifierManager
4. Leotheras
5. remaining T5/T6/SWP bosses

Resistance compensation belongs in a future `EncounterModifierManager`, not
in the normal gear database.


## Maulgar v1.1 transient-add ownership

Wild Fel Stalkers are not normal kill targets. Their ownership state is read
from the CMaNGOS charm relationship:

- `!creature->HasCharmer()` => hostile pickup/control target
- `creature->HasCharmer(warlockGuid)` => controlled by that exact Warlock

This makes Paladin release event-driven instead of timer-driven.

The Protection Paladin is therefore a transient bridge:
`spawn -> stabilize threat -> Warlock control -> release`.


## v1.2 post-DONE cleanup

`DONE` is now a conditional exit for Maulgar.

If `TYPE_MAULGAR_EVENT == DONE` but any Wild Fel Stalker entry 18847 is still
alive, Encounter AI remains active. It first breaks any remaining charm via
`Unit::Uncharm`, then assigns the hostile survivor as the cleanup target to
non-healer bots. The overlay exits only when the living Fel Stalker count is 0.


## v1.3 encounter-wide cast state

Blindeye rotation state must not live inside one bot. Each bot has its own AI
update loop, so a per-bot `nextInterruptRound` would immediately desynchronize.

v1.3 stores one transient state per live Map instance:
- active Spell pointer
- active round
- next round
- interrupted flag

A no-cast observation clears the active Spell marker. FAIL/NOT_STARTED/DONE
erases the state entirely.


## v1.5 formation overlay

Formation is a separate encounter utility, not part of Normal Rotation.

The Maulgar pull frame is captured from live positions and stored per Map
instance. Hard encounter actions always execute before formation movement.

Movement uses hysteresis and `MovePoint`; no absolute Gruul's Lair coordinates
are compiled into the module.


## v1.6 pull barrier

Pre-pull is now an explicit Encounter state even while the instance reports
NOT_STARTED.

Priority:

```text
absolute fixed-anchor readiness
    -> Misdirection arming
    -> Mage PULL_GO
    -> Hunter dedicated MD consumption
    -> normal encounter mechanics
```

Dedicated Tank/controller positions are never recomputed from boss coordinates.
General post-pull DPS/healer formations may still use relative geometry.
