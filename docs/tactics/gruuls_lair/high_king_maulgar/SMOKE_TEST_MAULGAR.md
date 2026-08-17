# High King Maulgar v1.6 Smoke Test

## Gate 0 — Fixed anchors configured

Before testing auto-pull:

- [ ] `MaulgarFixedPositions::PullAnchorsConfigured()` is true
- [ ] all 9 anchors were captured from the actual local map 565
- [ ] no tank/controller uses relative pull geometry

If anchors are not configured, auto-pull must remain disabled.

## Gate 1 — Exact pre-pull waiting positions

Expected:
- Feral MT -> fixed Maulgar anchor
- Prot Warrior -> fixed Blindeye anchor
- Balance -> fixed Kiggler anchor
- Mage Tank -> fixed Krosh anchor
- Warlock -> fixed Olm anchor
- Prot Paladin -> fixed Fel standby anchor
- Hunter A/B/C -> three fixed Hunter pull lanes

All designated bot actors remain behind the pull barrier and do not chase.

## Gate 2 — Misdirection target verification

Bot Mage path:

Before Frostbolt:
- Hunter A core threat-redirection target == Feral MT
- Hunter B core threat-redirection target == Prot Warrior
- Hunter C core threat-redirection target == Balance

Expected source log:
`[EncounterAI][Maulgar][Pull] Hunter ... Misdirection -> ... lane=N`

If any Hunter does not know spell 34477 or cannot apply it, Mage must NOT pull.

## Gate 3 — Mage pull + three-Hunter opening

Expected order:
1. all fixed actors ready
2. all 3 Misdirections active
3. Mage begins Frostbolt on Krosh (`PULL_GO`)
4. before waiting for an `IN_PROGRESS` polling cycle:
   - Hunter A starts Maulgar
   - Hunter B starts Blindeye
   - Hunter C starts Kiggler
   - Olm Warlock starts Searing Pain
5. encounter reaches IN_PROGRESS as these openers land
6. Hunter A remains on Maulgar only while MD remains active
7. Hunter B remains on Blindeye only while MD remains active
8. Hunter C remains on Kiggler only while MD remains active
9. Prot Paladin remains available for first Wild Fel Stalker

The Hunters are not released to generic `Blindeye -> Olm -> ...` kill order until
their own core Misdirection redirection target has cleared.

## Gate 4 — Threat landing

After the three opening lanes:
- Maulgar should travel to / stay on Feral MT
- Blindeye should travel to / stay on Prot Warrior
- Kiggler threat should be established on Balance
- Krosh should be on Mage Tank
- Olm should be on Warlock

No Hunter should own one of the five council mobs after the opener.

## Gate 5 — Existing mechanics regression

Still required:
- Blindeye strict 3-round interrupt chain
- Krosh Spellsteal
- Prot Paladin + Warlock parallel Fel pickup/Enslave
- already-controlled Warlock excluded from later Enslave assignments
- Fel -> melee Krosh
- Krosh dead -> Fel -> melee Maulgar
- Fel remains in Whirlwind
- Maulgar DONE -> surviving Fel Uncharm -> raid cleanup
- no Banish path
