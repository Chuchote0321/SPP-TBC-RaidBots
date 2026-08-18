#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class Creature;
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

    // Encounter-only formation overlay.
    //
    // Dedicated Tank/controller positions use calibrated absolute room anchors.
    // Dynamic geometry is retained only for general healer/ranged/melee spread
    // and Whirlwind escape, where exact fixed points are not required.
    class MaulgarFormationManager
    {
    public:
        static void Reset(PlayerbotAI* ai);

        // Capture the pull coordinate frame once all five council actors exist.
        static bool EnsureMaulgarFrame(
            PlayerbotAI* ai,
            Creature* maulgar,
            Creature* krosh,
            Creature* olm,
            Creature* kiggler,
            Creature* blindeye);

        // Dedicated tank/controller positioning.
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

        // Role helpers used by the Maulgar encounter.
        static bool IsMeleeFormationActor(uint32 lowGuid);

    private:
        MaulgarFormationManager() = delete;
    };
}
