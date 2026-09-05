#!/usr/bin/env python3
"""Verify the first native Karazhan boss-strategy port."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

REQUIRED_FILES = (
    "docs/KARAZHAN_BOSS_PORT_AUDIT.md",
    "docs/karazhan_phase1_port_manifest.yml",
    "playerbot/strategy/AiObjectContext.cpp",
    "playerbot/strategy/generic/KarazhanDungeonStrategies.cpp",
    "playerbot/strategy/generic/KarazhanDungeonStrategies.h",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Context.h",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Internal.h",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Movement.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Attumen.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Moroes.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Maiden.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Triggers.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Triggers.h",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Actions.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Actions.h",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Multipliers.cpp",
    "playerbot/strategy/raid/karazhan/KarazhanPhase1Multipliers.h",
)


def read(relative: str) -> str:
    path = ROOT / relative
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def require(
    errors: list[str],
    label: str,
    text: str,
    *tokens: str,
) -> None:
    for token in tokens:
        if token not in text:
            errors.append(f"{label} missing token: {token}")


def main() -> int:
    errors: list[str] = []

    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            errors.append(f"missing required file: {relative}")

    manifest = read("docs/karazhan_phase1_port_manifest.yml")
    require(
        errors,
        "manifest",
        manifest,
        "b949b50bfcdd4fab937781bac2d7765e39330e4b",
        "mode: file-by-file-semantics",
        "attumen-the-huntsman",
        "moroes",
        "maiden-of-virtue",
        "chess-event",
        "copied-karazhan-absolute-coordinate-table",
        "replacement-of-tbc-class-rotations",
    )

    ai_context = read("playerbot/strategy/AiObjectContext.cpp")
    require(
        errors,
        "AiObjectContext",
        ai_context,
        "KarazhanPhase1ActionContext",
        "KarazhanPhase1TriggerContext",
    )

    strategy_header = read(
        "playerbot/strategy/generic/KarazhanDungeonStrategies.h"
    )
    require(
        errors,
        "Karazhan strategy header",
        strategy_header,
        "InitCombatTriggers",
        "InitNonCombatTriggers",
        "InitCombatMultipliers",
    )

    strategy = read(
        "playerbot/strategy/generic/KarazhanDungeonStrategies.cpp"
    )
    require(
        errors,
        "Karazhan strategy",
        strategy,
        '"karazhan attumen phase one"',
        '"karazhan attumen phase two"',
        '"karazhan attumen transition"',
        '"karazhan moroes guest priority"',
        '"karazhan maiden tank position"',
        '"karazhan maiden ranged position"',
        '"karazhan maiden grounding totem"',
        "KarazhanAttumenTargetingMultiplier",
        "KarazhanAttumenStackMultiplier",
        "KarazhanAttumenDpsWaitMultiplier",
        "KarazhanMaidenFormationMultiplier",
        "KarazhanMaidenGroundingTotemMultiplier",
    )
    if "GetDefaultCombatActions" in strategy:
        errors.append("Karazhan raid Strategy replaces class defaults")

    context = read(
        "playerbot/strategy/raid/karazhan/KarazhanPhase1Context.h"
    )
    require(
        errors,
        "Karazhan contexts",
        context,
        'creators["karazhan handle attumen phase one"]',
        'creators["karazhan handle attumen phase two"]',
        'creators["karazhan prioritize moroes guest"]',
        'creators["karazhan position maiden tank"]',
        'creators["karazhan position maiden ranged"]',
        'creators["karazhan cast maiden grounding totem"]',
    )

    runtime = "\n".join(
        read(path)
        for path in (
            "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.cpp",
            "playerbot/strategy/raid/karazhan/KarazhanPhase1Movement.cpp",
            "playerbot/strategy/raid/karazhan/KarazhanPhase1Attumen.cpp",
            "playerbot/strategy/raid/karazhan/KarazhanPhase1Moroes.cpp",
            "playerbot/strategy/raid/karazhan/KarazhanPhase1Maiden.cpp",
        )
    )
    require(
        errors,
        "Karazhan runtime",
        runtime,
        "NPC_ATTUMEN_MOUNTED = 16152",
        "phase two therefore cannot require",
        "ATTUMEN_DPS_WAIT_MS = 5000",
        "MOROES_GUESTS",
        "s_markedGuest",
        "RAID_ICON_SKULL = 7",
        "SPELL_REPENTANCE = 29511",
        "MAIDEN_RANGED_SLOT_COUNT = 8",
        "MOVE_STEP = 5.0f",
        "GetRespawnCoord",
        "RaidCentroid",
        "UpdateAllowedPositionZ",
        "PathFinder path",
        "PATHFIND_NOPATH",
        "grounding totem",
    )

    multipliers = read(
        "playerbot/strategy/raid/karazhan/"
        "KarazhanPhase1Multipliers.cpp"
    )
    require(
        errors,
        "Karazhan multipliers",
        multipliers,
        'name == "tank assist"',
        'name == "dps assist"',
        "CastHealingSpellAction",
        "AttackAction",
        "CastSpellAction",
        'IsNamed(action, "karazhan handle attumen phase two")',
        'name == "grace of air totem"',
        'name == "tranquil air totem"',
        'name == "windfury totem"',
    )

    active = "\n".join(
        (
            strategy,
            runtime,
            multipliers,
            read(
                "playerbot/strategy/raid/karazhan/"
                "KarazhanPhase1Actions.cpp"
            ),
        )
    )

    # Known upstream room-coordinate tokens must not become an active
    # dependency of this port. Relative distances and spell/NPC IDs are valid.
    for token in (
        "ATTUMEN_TANK_POSITION",
        "MAIDEN_OF_VIRTUE_TANK_POSITION",
        "MAIDEN_OF_VIRTUE_RANGED_POSITIONS",
        "-11165.18f",
        "-11003.50f",
    ):
        if token in active:
            errors.append(f"active Karazhan code contains absolute token: {token}")

    if errors:
        print("KARAZHAN_PHASE1_PORT=FAIL")
        for error in errors:
            print(f" - {error}")
        return 1

    print("KARAZHAN_PHASE1_PORT=PASS")
    print("UPSTREAM_BASE=b949b50bfcdd4fab937781bac2d7765e39330e4b")
    print("PORT_MODE=FILE_BY_FILE_SEMANTICS")
    print("BOSSES=ATTUMEN,MOROES,MAIDEN")
    print("CLASS_ROTATION=RETAINED")
    print("POSITIONING=TARGET_RELATIVE_DYNAMIC")
    print("POSITION_VALIDATION=GROUND_LOS_PATHFINDER")
    print("ABSOLUTE_KARAZHAN_COORDINATES=ABSENT")
    print("CHESS_UPSTREAM_STRATEGY=ABSENT")
    return 0


if __name__ == "__main__":
    sys.exit(main())
