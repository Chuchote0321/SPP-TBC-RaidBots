#pragma once

class Creature;
class Player;
class PlayerbotAI;

namespace ai
{
    class GruulRuntime
    {
    public:
        static Creature* FindGruul(PlayerbotAI* ai);
        static bool IsEncounterInProgress(PlayerbotAI* ai);
        static bool IsShatterWindow(PlayerbotAI* ai, Creature* gruul = nullptr);

        static Player* ResolveMainTank(PlayerbotAI* ai);
        static Player* ResolveHurtfulSoaker(PlayerbotAI* ai);
        static bool IsMainTank(PlayerbotAI* ai);
        static bool IsHurtfulSoaker(PlayerbotAI* ai);
        static bool IsRangedOrHealer(PlayerbotAI* ai);

        static void SetEncounterTarget(PlayerbotAI* ai, Creature* gruul);

        // Return true only when a movement command was issued.
        static bool MaintainMainTankPosition(PlayerbotAI* ai, Creature* gruul);
        static bool MaintainHurtfulSoakerPosition(PlayerbotAI* ai, Creature* gruul);
        static bool MaintainRangedSpread(PlayerbotAI* ai, Creature* gruul);

        static bool ShouldDelayBloodlust(PlayerbotAI* ai);
        static bool ShouldLockMainTankMovement(PlayerbotAI* ai);

        static void Reset(PlayerbotAI* ai);

    private:
        GruulRuntime() = delete;
    };
}
