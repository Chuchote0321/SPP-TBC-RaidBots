#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class Creature;
class Player;
class Unit;
class PlayerbotAI;

namespace ai
{
    enum class MaulgarTankAnchorRole : uint8
    {
        Maulgar = 0,
        Blindeye,
        Olm,
        FelhunterStandby
    };

    // Preparation slots are role-relative positions in a frame derived from
    // the live council geometry and the raid approach direction. They are not
    // absolute map coordinates and do not require manual bot placement.
    enum class MaulgarPreparationRole : uint8
    {
        MaulgarTank = 0,
        BlindeyeTank,
        KigglerTank,
        KroshMage,
        OlmWarlock,
        FelhunterStandby,
        HunterMaulgar,
        HunterBlindeye,
        HunterKiggler,
        HealerBackline,
        RangedBackline,
        MeleeBackline
    };

    class MaulgarFormationManager
    {
    public:
        static void Reset(PlayerbotAI* ai);

        // Capture one stable encounter-local frame for the current Map
        // instance. The frame origin is the five-NPC council centroid; its
        // forward axis points from that centroid toward the raid centroid.
        static bool EnsureMaulgarFrame(
            PlayerbotAI* ai,
            Creature* maulgar,
            Creature* krosh,
            Creature* olm,
            Creature* kiggler,
            Creature* blindeye);

        // PREPARING: move only ai->GetBot() toward its deterministic role slot.
        // The movement is terrain/path validated and issued in <=5-yard steps.
        // Returns true after the current bot is inside its role tolerance.
        static bool MaintainPreparationPosition(
            PlayerbotAI* ai,
            MaulgarPreparationRole role);

        // Shared preparation barrier. Bots must occupy their generated slot.
        // Protected real-player Mage/Warlock specialists are never moved; they
        // satisfy a target-range and line-of-sight envelope instead.
        static bool IsPreparationActorReady(
            PlayerbotAI* ai,
            Player* actor,
            MaulgarPreparationRole role);

        static const char* PreparationRoleName(
            MaulgarPreparationRole role);

        // IN_PROGRESS dedicated role positioning. These methods use the same
        // live encounter frame; MaulgarFixedPositions is not consulted.
        static bool EnsureTankAnchor(
            PlayerbotAI* ai,
            Creature* target,
            MaulgarTankAnchorRole role);

        static bool EnsureKroshMagePosition(
            PlayerbotAI* ai,
            Creature* krosh);

        static bool EnsureKigglerPosition(
            PlayerbotAI* ai,
            Creature* kiggler);

        static bool EnsureOlmWarlockPosition(
            PlayerbotAI* ai,
            Creature* olm);

        // General raid geometry.
        static bool EnsureHealerPosition(PlayerbotAI* ai);
        static bool EnsureRangedPosition(PlayerbotAI* ai, Unit* target);
        static bool EnsureMeleePosition(PlayerbotAI* ai, Unit* target);

        // Deterministic Whirlwind evacuation. Returns true whenever the caller
        // should block normal melee rotation for the current tick.
        static bool HandleMaulgarWhirlwind(
            PlayerbotAI* ai,
            Creature* maulgar);

        static bool IsMeleeFormationActor(uint32 lowGuid);

    private:
        MaulgarFormationManager() = delete;
    };
}
