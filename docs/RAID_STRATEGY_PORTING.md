# TBC raid-strategy port policy

## Branch contract

The clean repository baseline remains `main`. Raid development is isolated from it:

| Purpose | Ref at start of this change |
|---|---|
| Clean local baseline | `main` @ `c453f2007079a18ef418ee7165471d12b48134c2` |
| Raid integration branch | `feature/raid-tactics` @ `0df173d68f50b21acd5c7c276766213598f6e052` |
| Delivery branch | `feature/gruuls-lair-phase1` |
| Upstream stable Playerbots | `cmangos/playerbots master` @ `d557e9873f2afbe9fd2fc8748363cfd756041d0d` |
| Upstream experimental source | `cmangos/playerbots AC-strategies` @ `a5b202b146338ac280551ce5e29158149c5e3d37` |
| CMaNGOS TBC build/core baseline | `cmangos/mangos-tbc master` @ `900ee47e8e4e0f6cf84f397e3df4d34cb6ac8557` |

The delivery pull request targets `feature/raid-tactics`, not `main`.

## Selective-port rule

`AC-strategies` must not be merged, rebased or cherry-picked as a branch. A port must:

1. identify exact source files and source commit;
2. extract only mechanics that remain valid for the local encounter architecture;
3. replace generic or placeholder behavior with CMaNGOS-specific implementation;
4. record accepted and rejected semantics in the manifest;
5. pass the scope verifier and TBC module build before merge.

## Gruul first-pass decision record

Inspected upstream files:

- `playerbot/strategy/generic/GruulsLairDungeonStrategies.cpp`
- `playerbot/strategy/generic/GruulsLairDungeonStrategies.h`
- `playerbot/strategy/actions/GruulsLairDungeonActions.h`
- `playerbot/strategy/triggers/GruulsLairDungeonTriggers.h`

Accepted:

- map ID 565;
- Gruul entry 19044;
- Ground Slam/Shatter trigger concept;
- spell identifiers 33525, 39187 and 33654.

Rejected:

- upstream branch-wide integration;
- `Flee(master target)` / `Flee(bot)` as a Shatter solution;
- generic strategy/action/trigger registration duplicated beside the local encounter router.

Local replacement:

- `GruulEncounter` owns encounter-state routing and tank assignments;
- `GruulShatterPlanner` computes max-min pairwise spacing;
- planned destinations are shared per map, so later bots avoid slots already reserved by earlier bots;
- normal rotation remains blocked through the dangerous window.

## Maulgar audit result

The existing native Maulgar implementation already contains all required first-pass mechanics:

- explicit five-target assignment policy;
- Krosh Spellsteal;
- Olm Enslave Demon reservation and Fel Stalker handoff;
- Blindeye interrupt rotation;
- deterministic council kill order.

It is retained as native code and is not replaced by the much smaller upstream Gruul-only prototype.

## Maulgar pull and Misdirection contract

The progression roster has exactly **three Hunters and three Misdirection lanes**. There is no fourth Hunter and no fourth Misdirection assignment.

| Hunter lane | Hunter attacks | Misdirection recipient |
|---|---|---|
| 0 | High King Maulgar | Feral Maulgar main tank |
| 1 | Blindeye the Seer | Protection Warrior Blindeye tank |
| 2 | Kiggler the Crazed | Balance Druid Kiggler tank |

Krosh and Olm are deliberately outside the Hunter Misdirection table:

- the Mage tank opens Krosh directly with Frostbolt and establishes its own ranged threat;
- the Warlock tank opens Olm directly with Searing Pain and establishes its own ranged threat;
- neither the Mage tank nor the Warlock tank is a Misdirection recipient.

The three-lane count and the absence of Krosh/Olm Misdirection lanes are enforced by `tools/raid/verify_port_scope.py`.