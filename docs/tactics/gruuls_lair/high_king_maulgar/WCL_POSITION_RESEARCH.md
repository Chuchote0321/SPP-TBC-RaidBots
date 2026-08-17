# WCL position research — High King Maulgar

## Goal

Replace manual local coordinate calibration with a robust formation learned from ~10 public Classic Fresh Maulgar kills from **February–May 2026**, prioritizing fights near ~2 minutes and raid compositions close to this project (multiple tanks, ~5 healers, Mage/Warlock special roles).

## Seed replay

User-provided reference: `qzJcKy6wf4Q8XDNY`, fight 2.

## Candidate pool currently indexed

The following public report-list entries are candidate logs, not yet accepted position samples. Fight duration/composition/positions must still be validated from report fight/events data:

1. Death — `Gruul + SSC` — 2026-05-20
2. Death — `Mag/Gruul Death` — 2026-05-13
3. Death — `Mag/Gruul/AQ40` — 2026-05-06
4. Death — `Gruul 4/28/26` — uploaded 2026-04-29
5. Death — `Mag / Gruul 4/14` — uploaded 2026-04-15
6. Death and Gravity — `(2026.05.29) Gruul`
7. Death and Gravity — `(2026.05.12) Gruul/Mag`
8. Death and Gravity — `(2026.05.08) Gruul/Mag`
9. Nazarick/Faldorf — `Gruul + Maggy jaaaaaeeeeee` — 2026-05-14
10. Nazarick/Faldorf — `Magtheridon + Gruul's` — 2026-05-09

Additional April/February/March candidates should replace May-heavy samples when positional event access yields enough valid fights.

## Required event data

Warcraft Logs Report events support `includeResources: true`. Position samples are extracted from resource `x/y` values. We must not treat WCL replay absolute coordinates as CMaNGOS world coordinates.

For each accepted fight:

1. verify High King Maulgar encounter and kill;
2. retain duration roughly 90–150 s where possible;
3. inspect roster/spec/role composition;
4. sample stable formation after the initial council split (e.g. 15–45 s) plus opening positions;
5. identify tank actors and healer actors from master data/specs and threat/healing behavior;
6. capture actor positions from positional resources;
7. capture council NPC positions to define the room frame.

## Coordinate normalization

Per fight:

- origin = council spawn centroid / stable council reference frame;
- x-axis = reproducible room axis inferred from council spawn geometry (not player facing);
- rotate/translate each fight into the same canonical frame;
- preserve distances in WCL replay units only after confirming scale consistency.

Aggregate each role anchor with median coordinates; reject outliers using MAD/IQR. For healers, retain clusters rather than forcing a single point.

## Output profile

Expected outputs:

- Maulgar tank anchor
- Blindeye tank anchor
- Kiggler balance-tank anchor
- Krosh mage-tank anchor
- Olm warlock-tank anchor
- Prot-Paladin Fel standby anchor
- three Hunter pull lanes
- healer cluster centroids / spread envelopes

The canonical WCL profile is then transformed at runtime into the local CMaNGOS room frame using the local council NPC spawn geometry. This avoids manual nine-character calibration while remaining more precise than simple boss-relative radius rules.

## Current blocker

The public indexed report pages expose candidate reports, but this environment cannot currently retrieve the authenticated/dynamic positional event payload for the supplied replay. **No numeric coordinates are committed until those positional events are actually read.**

## Reproducible export

`tools/wcl/export_wcl_fight.py` uses the official public API client-credentials flow and requests `events(... includeResources:true)`. Credentials are read only from environment variables; never commit the client secret. Exported JSON can then be converted to `data/tactics/gruuls_lair/high_king_maulgar/wcl_position_samples.csv` and aggregated with `tools/wcl/aggregate_maulgar_positions.py`.
