# Gruul's Lair tactics

- Map ID: 565
- Instance router: `GruulsLairTactics`
- Stable integration base: `feature/raid-tactics`
- Delivery branch: `feature/gruuls-lair-phase1`

## Implemented encounter overlays

### High King Maulgar

Native local strategy under `high_king_maulgar/`:

- Maulgar, Blindeye, Kiggler, Krosh and Olm assignments.
- Krosh Spellsteal.
- Olm Fel Stalker pickup, Enslave reservation and controlled-pet handoff.
- Blindeye three-round interrupt chain.
- Fixed kill order: Blindeye -> Olm -> Kiggler -> Krosh -> Maulgar.
- Pull coordination, fixed anchors, Whirlwind evacuation and post-kill Fel cleanup.

### Gruul

Selective semantics-only port under `gruul/`:

- CMaNGOS encounter state and NPC/spell identifiers.
- Main-tank and Hurtful Strike soaker assignments.
- Ground Slam/Shatter hard override.
- Shared pairwise-distance optimizer using live raid positions plus reserved destinations.

The experimental upstream `AC-strategies` branch is never merged wholesale. See
`docs/RAID_STRATEGY_PORTING.md` and `docs/raid_strategy_port_manifest.yml`.
