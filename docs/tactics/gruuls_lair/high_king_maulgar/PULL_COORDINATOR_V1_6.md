# Maulgar v1.6 — Fixed Pull & Misdirection Plan

## Why v1.5 relative tank geometry is retired

Tank/controller pull positions are now **absolute map-565 anchors**. The boss/add
is brought to the Tank; the Tank does not chase a relative point derived from
the live boss position.

Dynamic relative geometry remains only for ordinary healer/ranged/melee spread
after the pull.

## Five-way opening split

The three Hunter Misdirections cover the three council members whose assigned
Tanks otherwise benefit most from a clean remote pickup:

| Lane | Hunter | Misdirection recipient | Hunter attack target |
|---|---|---|---|
| 1 | BM Hunter A | Feral MT | High King Maulgar |
| 2 | BM Hunter B | Prot Warrior | Blindeye |
| 3 | Survival Hunter | Balance Tank | Kiggler |

The other two council members have dedicated ranged pullers:
- Mage Tank -> Krosh
- Warlock -> Olm

Prot Paladin waits at the fixed Felhunter standby anchor.

### Raid3 exact actors

- Bmthree 36 -> Feralthree 88 -> Maulgar
- Bmsix 48 -> Protwarthree 124 -> Blindeye
- Survivalthr 75 -> Balancethree 99 -> Kiggler
- Arcanethree 114 (or configured Mage/Human override) -> Krosh
- Destrothree 74 (or Human Chuchote) -> Olm
- Protpalthree 98 -> Fel standby

Raid1/Raid2 use their corresponding progression actors but the **same room
anchors**.

## Synchronization barrier

### Bot Mage Tank

1. All six Tank/controller actors reach fixed anchors.
2. All three Hunters reach their three fixed pull positions.
3. All three Hunters cast Misdirection on their exact assigned Tank.
4. Coordinator confirms the core threat-redirection target GUID for all three.
5. Mage Tank starts Frostbolt on Krosh: `PULL_GO`.
6. **Without waiting for `IN_PROGRESS`**, the three Hunters immediately lock
   Maulgar / Blindeye / Kiggler and begin consuming their full Misdirection
   opener; the Olm Warlock begins Searing Pain in the same pull epoch.
7. The instance transitions to `IN_PROGRESS` as the five-way opener lands.
8. Each Hunter remains locked to its own council target until the core threat
   redirection clears, then returns to the raid kill order.

This prevents:
- Hunter 1 shooting Maulgar once, then sending the remaining MD charges from
  Blindeye DPS to the Feral Tank;
- Hunter 2 leaking threat to the wrong Tank;
- Hunter 3 abandoning Kiggler before its Balance Tank has clean threat.

## Human Mage Tank

The server never casts for Game/Migu.

If a protected human Mage Tank is selected:
- the fixed Krosh Mage anchor still applies as a readiness check;
- the server **does not pre-cast Misdirection** while waiting;
- the human starts Krosh manually;
- on the first `IN_PROGRESS` tick the three bot Hunters immediately cast their
  assigned Misdirection and start their dedicated opener.

This is slightly later than the fully automated bot-Mage path but preserves the
"do not server-control human actors" rule.

## Misdirection implementation

No Hunter strategy action exists for Misdirection in the current bot code.
Encounter AI directly calls:

```cpp
ai->CastSpell(34477, assignedTank);
```

The cMaNGOS spell effect stores the target's GUID in the Hunter's hostile-ref
manager. Threat generation then reads that exact redirection target.

The coordinator therefore validates the actual core state rather than assuming
that a successful spell request means Misdirection is active.

## Fixed anchor calibration

`MaulgarFixedPositions.h` ships with `configured=false` values on purpose.

Do not invent room coordinates.

Use:
`sql/MAULGAR_FIXED_ANCHOR_CAPTURE_R3_READONLY.sql`

after manually placing the Raid3 calibration actors at the exact desired points.

Until all nine anchors are configured:
- fixed auto-position is disabled;
- automatic pull is disabled;
- v1.5 relative Tank positioning is **not** silently used as a substitute.
