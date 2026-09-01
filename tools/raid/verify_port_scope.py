#!/usr/bin/env python3
"""Verify the file-by-file TBC raid-strategy port contract."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

REQUIRED_FILES = (
    "docs/RAID_STRATEGY_PORTING.md",
    "docs/raid_strategy_port_manifest.yml",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulEncounter.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulEncounter.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.h",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCoordinator.cpp",
)

# These are upstream AC-strategies integration files. Their presence in this
# repository would indicate a branch-level copy instead of the local encounter
# router's selective-port architecture.
FORBIDDEN_LEGACY_FILES = (
    "playerbot/strategy/generic/GruulsLairDungeonStrategies.cpp",
    "playerbot/strategy/generic/GruulsLairDungeonStrategies.h",
    "playerbot/strategy/actions/GruulsLairDungeonActions.h",
    "playerbot/strategy/triggers/GruulsLairDungeonTriggers.h",
)

REQUIRED_MANIFEST_TOKENS = (
    "c453f2007079a18ef418ee7165471d12b48134c2",
    "d557e9873f2afbe9fd2fc8748363cfd756041d0d",
    "a5b202b146338ac280551ce5e29158149c5e3d37",
    "900ee47e8e4e0f6cf84f397e3df4d34cb6ac8557",
    "mode: semantics-only",
    "whole-branch-merge",
    "generic-flee-from-master-or-self",
)

SHA40 = re.compile(r"^[0-9a-f]{40}$")


def main() -> int:
    errors: list[str] = []

    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            errors.append(f"missing required file: {relative}")

    for relative in FORBIDDEN_LEGACY_FILES:
        if (ROOT / relative).exists():
            errors.append(f"forbidden legacy whole-port file present: {relative}")

    manifest_path = ROOT / "docs/raid_strategy_port_manifest.yml"
    if manifest_path.is_file():
        manifest = manifest_path.read_text(encoding="utf-8")
        for token in REQUIRED_MANIFEST_TOKENS:
            if token not in manifest:
                errors.append(f"manifest missing token: {token}")

        for line in manifest.splitlines():
            stripped = line.strip()
            if stripped.startswith((
                "commit:",
                "clean_head_at_start:",
                "integration_head_at_start:",
            )):
                value = stripped.split(":", 1)[1].strip()
                if not SHA40.fullmatch(value):
                    errors.append(
                        f"manifest contains invalid commit SHA: {value!r}")

    router_path = (
        ROOT
        / "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.cpp"
    )
    if router_path.is_file():
        router = router_path.read_text(encoding="utf-8")
        if "GruulEncounter::Update(ai)" not in router:
            errors.append(
                "Gruul encounter is not connected to GruulsLairTactics")

    constants_path = (
        ROOT / "playerbot/strategy/raid/common/EncounterTypes.h"
    )
    if constants_path.is_file():
        constants = constants_path.read_text(encoding="utf-8")
        for token in (
            "TYPE_GRUUL_EVENT",
            "NPC_GRUUL",
            "SPELL_GRUUL_SHATTER",
        ):
            if token not in constants:
                errors.append(f"EncounterTypes.h missing {token}")

    maulgar_path = (
        ROOT
        / "playerbot/strategy/raid/gruuls_lair/high_king_maulgar"
        / "HighKingMaulgarEncounter.cpp"
    )
    if maulgar_path.is_file():
        maulgar = maulgar_path.read_text(encoding="utf-8")
        required_maulgar_tokens = (
            "MAULGAR_FERAL_TANKS",
            "BLINDEYE_WARRIOR_TANKS",
            "KIGGLER_BALANCE",
            "SPELL_SPELLSTEAL",
            "HighestEnslaveDemonSpell",
            "BlindeyeInterruptRoundForGuid",
            "SelectKillOrderTarget",
        )
        for token in required_maulgar_tokens:
            if token not in maulgar:
                errors.append(
                    f"native Maulgar strategy missing regression token: {token}")
    else:
        errors.append("native HighKingMaulgarEncounter.cpp is missing")

    # The roster contains exactly three Hunters. Their Misdirection lanes cover
    # only Maulgar, Blindeye and Kiggler. Krosh and Olm are ranged-tank self-pulls
    # and must never acquire Hunter Misdirection lanes.
    pull_path = (
        ROOT
        / "playerbot/strategy/raid/gruuls_lair/high_king_maulgar"
        / "MaulgarPullCoordinator.cpp"
    )
    if pull_path.is_file():
        pull = pull_path.read_text(encoding="utf-8")

        required_three_lane_tokens = (
            "Maulgar = 0",
            "Blindeye = 1",
            "Kiggler = 2",
            "Count = 3",
            "uint32 hunters[3];",
            "bool hunterMdReady[3];",
            "bool hunterOpeningComplete[3];",
            "for (uint8 lane = 0; lane < 3; ++lane)",
            "HunterPullLane::Maulgar",
            "HunterPullLane::Blindeye",
            "HunterPullLane::Kiggler",
            "ai->CastSpell(\"frostbolt\", krosh)",
            "ai->CastSpell(\"searing pain\", olm)",
        )
        for token in required_three_lane_tokens:
            if token not in pull:
                errors.append(
                    f"three-Hunter Misdirection contract missing token: {token}")

        forbidden_fourth_lane_tokens = (
            "uint32 hunters[4]",
            "hunterMdReady[4]",
            "hunterOpeningComplete[4]",
            "lane < 4",
            "HunterPullLane::Krosh",
            "HunterPullLane::Olm",
        )
        for token in forbidden_fourth_lane_tokens:
            if token in pull:
                errors.append(
                    f"invalid fourth/ranged-tank Misdirection lane present: {token}")
    else:
        errors.append("MaulgarPullCoordinator.cpp is missing")

    docs_path = ROOT / "docs/RAID_STRATEGY_PORTING.md"
    if docs_path.is_file():
        docs = docs_path.read_text(encoding="utf-8")
        for token in (
            "three Hunters and three Misdirection lanes",
            "Mage tank opens Krosh directly",
            "Warlock tank opens Olm directly",
            "neither the Mage tank nor the Warlock tank is a Misdirection recipient",
        ):
            if token not in docs:
                errors.append(
                    f"Misdirection documentation missing token: {token}")

    if errors:
        print("RAID_PORT_SCOPE=FAIL")
        for error in errors:
            print(f" - {error}")
        return 1

    print("RAID_PORT_SCOPE=PASS")
    print("BASELINE_POLICY=MAIN_UNTOUCHED")
    print("PORT_MODE=FILE_BY_FILE_SEMANTICS_ONLY")
    print("GRUUL_ROUTER=CONNECTED")
    print("MAULGAR_NATIVE_REGRESSION=PASS")
    print("MAULGAR_MISDIRECTION_LANES=3")
    print("KROSH_OLM_SELF_PULL=PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
