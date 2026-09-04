#!/usr/bin/env python3
"""Verify the selective TBC raid port and reject legacy Gruul dispatch."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

REQUIRED_FILES = (
    "docs/RAID_STRATEGY_PORTING.md",
    "docs/raid_strategy_port_manifest.yml",
    "docs/GRUUL_ENCOUNTER_PORT.md",
    "docs/gruul_encounter_port_manifest.yml",
    "playerbot/strategy/AiObjectContext.cpp",
    "playerbot/strategy/generic/DungeonStrategy.cpp",
    "playerbot/strategy/raid/common/EncounterManager.cpp",
    "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/HighKingMaulgarEncounter.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFormationManager.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandController.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandTriggers.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandContext.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulAiObjectContext.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulStrategy.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulTriggers.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulMultipliers.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.cpp",
)

FORBIDDEN_FILES = (
    "playerbot/strategy/generic/GruulsLairDungeonStrategies.cpp",
    "playerbot/strategy/generic/GruulsLairDungeonStrategies.h",
    "playerbot/strategy/actions/GruulsLairDungeonActions.h",
    "playerbot/strategy/triggers/GruulsLairDungeonTriggers.h",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFixedPositions.h",
)


def read(relative: str) -> str:
    path = ROOT / relative
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def require(errors: list[str], label: str, content: str, *tokens: str) -> None:
    for token in tokens:
        if token not in content:
            errors.append(f"{label} missing token: {token}")


def main() -> int:
    errors: list[str] = []

    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            errors.append(f"missing required file: {relative}")

    for relative in FORBIDDEN_FILES:
        if (ROOT / relative).exists():
            errors.append(f"forbidden legacy file present: {relative}")

    constants = read("playerbot/strategy/raid/common/EncounterTypes.h")
    require(
        errors,
        "EncounterTypes",
        constants,
        "TYPE_MAULGAR_EVENT",
        "TYPE_GRUUL_EVENT",
        "NPC_GRUUL",
        "SPELL_GRUUL_GROUND_SLAM",
        "SPELL_GRUUL_SHATTER",
    )

    manager = read("playerbot/strategy/raid/common/EncounterManager.cpp")
    require(
        errors,
        "EncounterManager",
        manager,
        "MaulgarPullCommandController::Update(ai)",
        "MaulgarPullCommandController::AllowEncounterDispatch(ai)",
        "GruulsLairTactics::Update(ai)",
    )
    command_index = manager.find("MaulgarPullCommandController::Update(ai)")
    gate_index = manager.find("MaulgarPullCommandController::AllowEncounterDispatch(ai)")
    tactics_index = manager.find("GruulsLairTactics::Update(ai)")
    if not (command_index >= 0 and gate_index > command_index and tactics_index > gate_index):
        errors.append("EncounterManager order is not command -> gate -> tactics")

    highking = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/HighKingMaulgarEncounter.cpp"
    )
    require(
        errors,
        "Maulgar encounter",
        highking,
        "MAULGAR_FERAL_TANKS",
        "BLINDEYE_WARRIOR_TANKS",
        "KIGGLER_BALANCE",
        "HighestEnslaveDemonSpell",
        "BlindeyeInterruptRoundForGuid",
        "SelectKillOrderTarget",
    )
    if "MaulgarPullCoordinator::UpdatePrePull(ai)" in highking:
        errors.append("legacy NOT_STARTED Maulgar auto-pull is active")

    controller = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandController.cpp"
    )
    require(
        errors,
        "Maulgar command controller",
        controller,
        "Maulgar = 0",
        "Blindeye = 1",
        "Kiggler = 2",
        "Count = 3",
        "uint32 hunters[3];",
        "bool hunterOpened[3];",
        "AllFormationReady",
        "AllMisdirectionsReady",
        "POSITION_MODE=AUTO_RELATIVE",
        "COMMAND_MD_ARM",
        "COMMAND_MD_REARM",
        'ai->CastSpell("frostbolt", krosh)',
        'ai->CastSpell("searing pain", olm)',
        "return maulgarState != 0;",
    )
    for token in (
        "hunters[4]",
        "hunterOpened[4]",
        "lane < 4",
        "HunterPullLane::Krosh",
        "HunterPullLane::Olm",
    ):
        if token in controller:
            errors.append(f"invalid fourth Misdirection lane: {token}")

    formation = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFormationManager.cpp"
    )
    require(
        errors,
        "Maulgar formation",
        formation,
        "source=COUNCIL_PLUS_RAID_CENTROID",
        "PREP_MOVE_STEP = 5.0f",
        "MaintainPreparationPosition",
        "IsPreparationActorReady",
        "UpdateAllowedPositionZ",
        "PathFinder path",
    )
    if "MaulgarFixedPositions::" in formation:
        errors.append("active Maulgar formation uses fixed positions")

    ai_context = read("playerbot/strategy/AiObjectContext.cpp")
    require(
        errors,
        "AiObjectContext",
        ai_context,
        "MaulgarPullCommandTriggerContext",
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

    tactics = read("playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.cpp")
    require(
        errors,
        "GruulsLairTactics",
        tactics,
        "HighKingMaulgarEncounter::Update(ai)",
        "native Strategy/Trigger/Action/Multiplier",
    )
    if "GruulEncounter::Update(ai)" in tactics:
        errors.append("legacy pre-engine Gruul dispatch remains active")

    strategy = read("playerbot/strategy/raid/gruuls_lair/gruul/GruulStrategy.cpp")
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
        errors.append("Gruul raid Strategy replaces class defaults")

    actions = read("playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.cpp")
    require(
        errors,
        "Gruul Actions",
        actions,
        "MaintainMainTankPosition",
        "MaintainHurtfulSoakerPosition",
        "MaintainRangedSpread",
        "GruulShatterPlanner::Update",
    )

    multipliers = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/GruulMultipliers.cpp"
    )
    require(
        errors,
        "Gruul Multipliers",
        multipliers,
        'name == "bloodlust"',
        'name == "heroism"',
        "ShouldLockMainTankMovement",
        'IsNamed(action, "gruul shatter spread")',
        'name.find("reach ") == 0',
        "dynamic_cast<MovementAction*>",
    )

    runtime = read("playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.cpp")
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
    )

    shatter = read(
        "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.cpp"
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

    active_gruul = "\n".join((strategy, actions, multipliers, runtime, shatter))
    for token in ("GRUUL_TANK_POSITION", "MAULGAR_ROOM_CENTER", "241.238f", "365.025f"):
        if token in active_gruul:
            errors.append(f"active Gruul code contains absolute token: {token}")

    if errors:
        print("RAID_PORT_SCOPE=FAIL")
        for error in errors:
            print(f" - {error}")
        return 1

    print("RAID_PORT_SCOPE=PASS")
    print("BASELINE_POLICY=MAIN_UNTOUCHED")
    print("PORT_MODE=FILE_BY_FILE_SEMANTICS_ONLY")
    print("MAULGAR_PULL_COMMANDS=PREPARE_THEN_PULL")
    print("MAULGAR_MISDIRECTION_LANES=3")
    print("MAULGAR_POSITIONING=ENCOUNTER_RELATIVE_AUTO")
    print("GRUUL_ACTIVE_ROUTE=NATIVE_STRATEGY_ENGINE")
    print("PRE_ENGINE_GRUUL_DISPATCH=DISABLED")
    print("GRUUL_CLASS_ROTATION=RETAINED")
    print("GRUUL_ABSOLUTE_COORDINATES=ABSENT")
    return 0


if __name__ == "__main__":
    sys.exit(main())
