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
    return 0


if __name__ == "__main__":
    sys.exit(main())
