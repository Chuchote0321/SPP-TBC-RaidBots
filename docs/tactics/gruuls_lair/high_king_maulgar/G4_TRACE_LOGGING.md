# G4 Maulgar Encounter trace logging

G4 uses the existing CMaNGOS custom-log channel.

## Runtime config

Set in the `mangosd.conf` used for G4:

```ini
CustomLogFile = "RaidEncounter.log"
```

The encounter layer emits lines beginning with:

```text
[RAID_ENCOUNTER]
```

Every trace line includes map, instance, current bot, low GUID, encounter and event.

## Initial G4 events

- `MAP_ENTER`
- `ENCOUNTER_STATE`
- `ROLE_ASSIGN`
- `HUMAN_PROTECTED`
- `PULL_ARMED`
- `PULL_GO`
- `MISDIRECT_CAST`
- `MISDIRECT_CLEAR`
- `OLM_OPENER`

State transitions and role assignments are deduplicated to avoid per-tick spam.
Action events are logged when the corresponding action is actually issued.

Keep normal `Server.log` together with `RaidEncounter.log`: the first proves
core/runtime health, the second proves encounter decision flow.