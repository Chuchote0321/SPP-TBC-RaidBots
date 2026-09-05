#pragma once

class Player;
class PlayerbotAI;
class Unit;

namespace ai
{
    class KarazhanPhase1Runtime
    {
    public:
        static constexpr uint32 MAP_KARAZHAN = 532;

        static Unit* FindTarget(PlayerbotAI* ai, const char* name);
        static Unit* FindMountedAttumen(PlayerbotAI* ai);

        static bool IsAttumenPhaseOne(PlayerbotAI* ai);
        static bool IsAttumenPhaseTwo(PlayerbotAI* ai);
        static bool IsMoroesActive(PlayerbotAI* ai);
        static bool HasMoroesGuest(PlayerbotAI* ai);
        static bool IsMaidenActive(PlayerbotAI* ai);

        static bool IsMainTank(PlayerbotAI* ai);
        static bool IsAssistTank(PlayerbotAI* ai);
        static bool IsRangedOrHealer(PlayerbotAI* ai);
        static bool IsCoordinator(PlayerbotAI* ai);

        static void SetEncounterTarget(PlayerbotAI* ai, Unit* target);

        // Each handler returns true only when it issued a spell or movement
        // action that should consume the current Engine tick. Setting a target
        // alone returns false so the existing TBC class Strategy may continue.
        static bool HandleAttumenPhaseOne(PlayerbotAI* ai);
        static bool HandleAttumenPhaseTwo(PlayerbotAI* ai);
        static bool ObserveAttumenTransition(PlayerbotAI* ai);
        static bool PrioritizeMoroesGuest(PlayerbotAI* ai);
        static bool MaintainMaidenTankPosition(PlayerbotAI* ai);
        static bool MaintainMaidenRangedPosition(PlayerbotAI* ai);
        static bool CastMaidenGroundingTotem(PlayerbotAI* ai);

        static bool ShouldSuppressAttumenAutomaticTargeting(PlayerbotAI* ai);
        static bool ShouldKeepAttumenStacked(PlayerbotAI* ai);
        static bool ShouldWaitForAttumenTank(PlayerbotAI* ai);
        static bool ShouldSuppressMaidenFormation(PlayerbotAI* ai);
        static bool ShouldReserveMaidenAirTotem(PlayerbotAI* ai);

        static void Reset(PlayerbotAI* ai);

    private:
        KarazhanPhase1Runtime() = delete;
    };
}
