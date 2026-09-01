# Gruul Encounter

Status: **first executable implementation; compile and in-game smoke validation required.**

## Locked behavior

- Uses CMaNGOS instance state `TYPE_GRUUL_EVENT = 1`.
- Feral progression tank is the main tank.
- Protection Warrior is the preferred Hurtful Strike soaker; Protection Paladin is the fallback.
- Healer target selection is not overwritten.
- Ground Slam/Shatter suppresses normal class rotation until the spread window ends.
- Every bot selects a slot by scoring current raid-member positions and already-reserved bot slots.
- The primary objective is max-min pairwise separation; quadratic penalties reject locally crowded points.
- A Stoned bot no longer receives impossible movement orders, but normal rotation remains blocked until Shatter resolves.

## Source provenance

Only encounter identifiers and the Ground Slam/Shatter trigger concept were taken from
`cmangos/playerbots@a5b202b146338ac280551ce5e29158149c5e3d37`.
The upstream `Flee(master target/self)` implementation was not copied because it does not
measure teammate separation.

Authoritative CMaNGOS spell sequence was checked against
`cmangos/mangos-tbc@900ee47e8e4e0f6cf84f397e3df4d34cb6ac8557`,
`boss_gruul.cpp`.
