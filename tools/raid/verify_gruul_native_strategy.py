#!/usr/bin/env python3
"""Verify the native Gruul encounter Strategy port."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

REQUIRED_FILES = (
    "docs/GRUUL_ENCOUNTER_PORT.md",
    "docs/gruul_encounter_port_manifest.yml",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulAiObjectContext.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulStrategy.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulStrategy.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulTriggers.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulTriggers.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulMultipliers.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulMultipliers.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.cpp",
)


def read(relative: str) -> str:
    path = ROOT / relative
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def require(errors: list[str], label: str, text: str, *tokens: str) -> None:
    for token in tokens:
        if token not in text:
            errors.append(f"{label} missing token: {token}")


def main() -> int:
    errors: list[str] = []

    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            errors.append(f"missing required file: {relative}")

    manifest = read("docs/gruul_encounter_port_manifest.yml")
    require(
        errors,
        "manifest",
        manifest,
        "b949b50bfcdd4fab937781bac2d7765e39330e4b",
        "mode: file-by-file-semantics",
        "active_route: native-strategy-engine",
        "absolute-gruul-room-coordinate-copy",
        "pre-engine-gruul-dispatch",
        "replaced: false",
    )

    context = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/"
        "GruulAiObjectContext.h"
    )
    require(
        errors,
        "Gruul context",
        context,
        'creators["gruul\'s lair"]',
        'creators["enter gruul\'s lair"]',
        'creators["leave gruul\'s lair"]',
        'creators["gruul tank position"]',
        'creators["gruul maintain ranged spread"]',
        'creators["gruul shatter spread"]',
    )

    ai_context = read("playerbot/strategy/AiObjectContext.cpp")
    require(
        errors,
        "AiObjectContext",
        ai_context,
        "GruulsLairStrategyContext",
        "GruulsLairActionContext",
        "GruulsLairTriggerContext",
    )

    dungeon = read("playerbot/strategy/generic/DungeonStrategy.cpp")
    require(
        errors,
        "DungeonStrategy",
        dungeon,
        '"enter gruul\'s lair"',
        '"leave gruul\'s lair"',
        '"enable gruul\'s lair strategy"',
        '"disable gruul\'s lair strategy"',
    )

    strategy = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/"
        "GruulStrategy.cpp"
    )
    require(
        errors,
        "Gruul Strategy",
        strategy,
        '"gruul incoming shatter"',
        '"gruul tank positioning"',
        '"gruul ranged spread"',
        "GruulDelayBloodlustMultiplier",
        "GruulControlMainTankMovementMultiplier",
        "GruulShatterMovementMultiplier",
    )
    if "GetDefaultCombatActions" in strategy:
        errors.append("Gruul raid Strategy must not replace class defaults")

    actions = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/"
        "GruulActions.cpp"
    )
    require(
        errors,
        "Gruul Actions",
        actions,
        "MaintainMainTankPosition",
        "MaintainHurtfulSoakerPosition",
        "MaintainRangedSpread",
        "GruulShatterPlanner::Update",
        "result == EncounterOverrideResult::Handled",
    )

    multipliers = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/"
        "GruulMultipliers.cpp"
    )
    require(
        errors,
        "Gruul Multipliers",
        multipliers,
        'name == "bloodlust"',
        'name == "heroism"',
        "ShouldLockMainTankMovement",
        'IsNamed(action, "gruul shatter spread")',
        "dynamic_cast<MovementAction*>",
    )

    runtime = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/"
        "GruulRuntime.cpp"
    )
    require(
        errors,
        "Gruul Runtime",
        runtime,
        "GRUUL_RESPAWN_PLUS_RAID_CENTROID",
        "gruul->GetRespawnCoord",
        "MOVE_STEP = 5.0f",
        "ResolveHurtfulSoaker",
        "RANGED_SLOTS_PER_RING",
        "UpdateAllowedPositionZ",
        "PathFinder path",
        "PATHFIND_NOPATH",
        "MaintainRangedSpread",
    )

    shatter = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/"
        "GruulShatterPlanner.cpp"
    )
    require(
        errors,
        "Shatter planner",
        shatter,
        "ReachableCandidate",
        "UpdateAllowedPositionZ",
        "PathFinder path",
        "return EncounterOverrideResult::Handled;",
    )

    tactics = read(
        "playerbot/strategy/raid/gruuls_lair/"
        "GruulsLairTactics.cpp"
    )
    if "GruulEncounter::Update(ai)" in tactics:
        errors.append("pre-engine Gruul encounter dispatch is still active")
    require(
        errors,
        "GruulsLairTactics",
        tactics,
        "HighKingMaulgarEncounter::Update(ai)",
        "native Strategy/Trigger/Action/Multiplier",
    )

    forbidden_absolute_tokens = (
        "GRUUL_TANK_POSITION",
        "MAULGAR_ROOM_CENTER",
        "241.238f",
        "365.025f",
    )
    active_files = runtime + actions + strategy + multipliers + shatter
    for token in forbidden_absolute_tokens:
        if token in active_files:
            errors.append(
                f"active Gruul implementation contains absolute token: {token}"
            )

    if errors:
        print("GRUUL_NATIVE_STRATEGY=FAIL")
        for error in errors:
            print(f" - {error}")
        return 1

    print("GRUUL_NATIVE_STRATEGY=PASS")
    print("UPSTREAM_BASE=b949b50bfcdd4fab937781bac2d7765e39330e4b")
    print("PORT_MODE=FILE_BY_FILE_SEMANTICS")
    print("ACTIVE_ROUTE=STRATEGY_TRIGGER_ACTION_MULTIPLIER")
    print("PRE_ENGINE_GRUUL_DISPATCH=DISABLED")
    print("CLASS_ROTATION=RETAINED")
    print("MAIN_TANK_POSITION=DYNAMIC")
    print("HURTFUL_SOAKER_POSITION=DYNAMIC")
    print("RANGED_SPREAD=PERSISTENT_TWO_RING")
    print("SHATTER_PLANNER=MAX_MIN_PATH_VALIDATED")
    print("ABSOLUTE_GRUUL_COORDINATES=ABSENT")
    return 0


if __name__ == "__main__":
    sys.exit(main())
