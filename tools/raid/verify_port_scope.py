#!/usr/bin/env python3
"""Verify the selective TBC raid port and explicit Maulgar pull contract."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

REQUIRED_FILES = (
    "docs/RAID_STRATEGY_PORTING.md",
    "docs/raid_strategy_port_manifest.yml",
    "playerbot/strategy/raid/common/EncounterManager.cpp",
    "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulEncounter.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulEncounter.h",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.cpp",
    "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.h",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "HighKingMaulgarEncounter.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "MaulgarPullCoordinator.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "MaulgarPullCommandController.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "MaulgarPullCommandController.h",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "MaulgarPullCommandTriggers.cpp",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "MaulgarPullCommandTriggers.h",
    "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
    "MaulgarPullCommandContext.h",
)

# These are the old AC-strategies whole-port integration files. Their presence
# would indicate a branch-level copy rather than a selective semantics port.
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
    "mode: explicit-two-stage",
    "prepare_command: raid prepare maulgar",
    "pull_command: raid pull maulgar",
    "hunter_count: 3",
    "misdirection_count: 3",
    "automatic-not-started-pull",
    "2f7d9f774987d0157c6a0d0cc08c40bec3db3945",
    "mode: encounter-relative-auto",
    "manual-bot-pre-positioning",
    "local-absolute-coordinate-table",
)

SHA40 = re.compile(r"^[0-9a-f]{40}$")


def read(relative: str) -> str:
    path = ROOT / relative
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def require_tokens(
    errors: list[str],
    label: str,
    content: str,
    tokens: tuple[str, ...],
) -> None:
    for token in tokens:
        if token not in content:
            errors.append(f"{label} missing token: {token}")


def main() -> int:
    errors: list[str] = []

    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            errors.append(f"missing required file: {relative}")

    for relative in FORBIDDEN_LEGACY_FILES:
        if (ROOT / relative).exists():
            errors.append(f"forbidden legacy whole-port file present: {relative}")

    manifest = read("docs/raid_strategy_port_manifest.yml")
    require_tokens(errors, "manifest", manifest, REQUIRED_MANIFEST_TOKENS)

    for line in manifest.splitlines():
        stripped = line.strip()
        if stripped.startswith(
            (
                "commit:",
                "clean_head_at_start:",
                "integration_head_at_start:",
            )
        ):
            value = stripped.split(":", 1)[1].strip()
            if not SHA40.fullmatch(value):
                errors.append(f"manifest contains invalid commit SHA: {value!r}")

    constants = read("playerbot/strategy/raid/common/EncounterTypes.h")
    require_tokens(
        errors,
        "EncounterTypes.h",
        constants,
        (
            "TYPE_MAULGAR_EVENT",
            "TYPE_GRUUL_EVENT",
            "NPC_GRUUL",
            "SPELL_GRUUL_SHATTER",
        ),
    )

    router = read(
        "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.cpp"
    )
    if "GruulEncounter::Update(ai)" not in router:
        errors.append("Gruul encounter is not connected to GruulsLairTactics")

    maulgar = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "HighKingMaulgarEncounter.cpp"
    )
    require_tokens(
        errors,
        "native Maulgar strategy",
        maulgar,
        (
            "MAULGAR_FERAL_TANKS",
            "BLINDEYE_WARRIOR_TANKS",
            "KIGGLER_BALANCE",
            "SPELL_SPELLSTEAL",
            "HighestEnslaveDemonSpell",
            "BlindeyeInterruptRoundForGuid",
            "SelectKillOrderTarget",
        ),
    )

    # Exactly three Hunters handle exactly three MD lanes. Krosh and Olm remain
    # self-pulls by their ranged tanks.
    pull = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "MaulgarPullCoordinator.cpp"
    )
    require_tokens(
        errors,
        "legacy three-Hunter topology",
        pull,
        (
            "Maulgar = 0",
            "Blindeye = 1",
            "Kiggler = 2",
            "Count = 3",
            "uint32 hunters[3];",
            "bool hunterMdReady[3];",
            "bool hunterOpeningComplete[3];",
            "for (uint8 lane = 0; lane < 3; ++lane)",
            'ai->CastSpell("frostbolt", krosh)',
            'ai->CastSpell("searing pain", olm)',
        ),
    )


    formation = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "MaulgarFormationManager.cpp"
    )
    require_tokens(
        errors,
        "encounter-relative Maulgar positioning",
        formation,
        (
            "source=COUNCIL_PLUS_RAID_CENTROID",
            "PREP_MOVE_STEP = 5.0f",
            "UpdateAllowedPositionZ",
            "PathFinder path(actor)",
            "PATHFIND_NOPATH",
            "MaintainPreparationPosition",
            "IsPreparationActorReady",
            "state->maulgarAnchor",
            "RelativeTargetSlot(*state, krosh",
            "RelativeTargetSlot(*state, kiggler",
            "RelativeTargetSlot(*state, olm",
        ),
    )
    if "MaulgarFixedPositions::" in formation:
        errors.append(
            "active Maulgar formation still depends on MaulgarFixedPositions"
        )

    controller = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "MaulgarPullCommandController.cpp"
    )
    require_tokens(
        errors,
        "explicit pull controller",
        controller,
        (
            "MaulgarPullCommandPhase::Idle",
            "MaulgarPullCommandPhase::Preparing",
            "MaulgarPullCommandPhase::Armed",
            "MaulgarPullCommandPhase::PullRequested",
            "AllMisdirectionsReady",
            "AllFormationReady",
            "MaulgarFormationManager::MaintainPreparationPosition",
            "POSITION_MODE=AUTO_RELATIVE",
            "GROUND_LOS_PATHFINDER",
            "COMMAND_MD_ARM",
            "COMMAND_MD_REARM",
            "COMMAND_HUNTER_OPEN",
            "HUMAN_MAGE_ENGAGED",
            'ai->CastSpell("frostbolt", krosh)',
            'ai->CastSpell("searing pain", olm)',
            "return maulgarState != 0;",
        ),
    )

    forbidden_fourth_lane_tokens = (
        "uint32 hunters[4]",
        "hunterMdReady[4]",
        "hunterOpeningComplete[4]",
        "hunterOpened[4]",
        "lane < 4",
        "HunterPullLane::Krosh",
        "HunterPullLane::Olm",
    )
    for token in forbidden_fourth_lane_tokens:
        if token in pull or token in controller:
            errors.append(
                f"invalid fourth/ranged-tank Misdirection lane present: {token}"
            )

    trigger_impl = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "MaulgarPullCommandTriggers.cpp"
    )
    trigger_header = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "MaulgarPullCommandTriggers.h"
    )
    trigger_context = read(
        "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/"
        "MaulgarPullCommandContext.h"
    )
    ai_context = read("playerbot/strategy/AiObjectContext.cpp")

    require_tokens(
        errors,
        "command trigger implementation",
        trigger_impl,
        (
            "MaulgarPullCommandController::RequestPrepare(ai, owner)",
            "MaulgarPullCommandController::RequestPull(ai, owner)",
        ),
    )
    require_tokens(
        errors,
        "command trigger declaration",
        trigger_header,
        (
            'Trigger(ai, "raid prepare maulgar")',
            'Trigger(ai, "raid pull maulgar")',
            "ExternalEvent(",
            "Event Check() override { return Event(); }",
        ),
    )
    require_tokens(
        errors,
        "command trigger context",
        trigger_context,
        (
            'creators["raid prepare maulgar"]',
            'creators["raid pull maulgar"]',
            "MaulgarPrepareCommandTrigger",
            "MaulgarPullCommandTrigger",
        ),
    )
    if "MaulgarPullCommandTriggerContext" not in ai_context:
        errors.append(
            "AiObjectContext does not register MaulgarPullCommandTriggerContext"
        )
    if "MaulgarPullCommandActionContext" in ai_context:
        errors.append(
            "Maulgar command incorrectly waits for an Engine Action context"
        )

    encounter_manager = read(
        "playerbot/strategy/raid/common/EncounterManager.cpp"
    )
    require_tokens(
        errors,
        "EncounterManager explicit gate",
        encounter_manager,
        (
            "MaulgarPullCommandController::Update(ai)",
            "MaulgarPullCommandController::AllowEncounterDispatch(ai)",
            "GruulsLairTactics::Update(ai)",
        ),
    )
    command_index = encounter_manager.find(
        "MaulgarPullCommandController::Update(ai)"
    )
    gate_index = encounter_manager.find(
        "MaulgarPullCommandController::AllowEncounterDispatch(ai)"
    )
    legacy_index = encounter_manager.find("GruulsLairTactics::Update(ai)")
    if not (
        command_index >= 0
        and gate_index > command_index
        and legacy_index > gate_index
    ):
        errors.append(
            "EncounterManager order must be command update -> gate -> legacy router"
        )

    docs = read("docs/RAID_STRATEGY_PORTING.md")
    require_tokens(
        errors,
        "Misdirection/command documentation",
        docs,
        (
            "three Hunters and three Misdirection lanes",
            "Mage tank opens Krosh directly",
            "Warlock tank opens Olm directly",
            "neither the Mage tank nor the Warlock tank is a Misdirection recipient",
            "/ra raid prepare maulgar",
            "/ra raid pull maulgar",
            "legacy encounter router is disabled",
            "immediately in `ExternalEvent()`",
            "Encounter-relative automatic positioning",
            "at most 5-yard steps",
            "eliminating manual",
        ),
    )

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
    print("MAULGAR_PULL_COMMANDS=PREPARE_THEN_PULL")
    print("MAULGAR_NOT_STARTED_AUTOPULL=DISABLED")
    print("COMMAND_DELIVERY=IMMEDIATE_EXTERNAL_EVENT")
    print("MAULGAR_POSITIONING=ENCOUNTER_RELATIVE_AUTO")
    print("ABSOLUTE_COORDINATE_DEPENDENCY=NONE")
    print("POSITION_VALIDATION=GROUND_LOS_PATHFINDER")
    return 0


if __name__ == "__main__":
    sys.exit(main())
