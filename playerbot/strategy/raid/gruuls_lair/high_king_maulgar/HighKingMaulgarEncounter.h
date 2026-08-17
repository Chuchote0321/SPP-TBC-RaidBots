#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class Creature;
class Unit;

namespace ai
{
    class PlayerbotAI;

    class HighKingMaulgarEncounter
    {
    public:
        static EncounterOverrideResult Update(PlayerbotAI* ai);

        // Exposed as stateless encounter utilities for resolver helpers.
        static Creature* FindFelhunterControlledBy(PlayerbotAI* ai, Unit* charmer);
        static bool HasAnyControlledFelhunter(PlayerbotAI* ai, Unit* charmer);

    private:
        static Creature* FindCreature(PlayerbotAI* ai, uint32 entry);
        static Creature* FindUncontrolledFelhunter(PlayerbotAI* ai);
        static Creature* FindAnyLivingFelhunter(PlayerbotAI* ai);
        static uint32 CountLivingFelhunter(PlayerbotAI* ai);
        static uint32 HighestEnslaveDemonSpell(PlayerbotAI* ai);
        static void SetEncounterTarget(PlayerbotAI* ai, Unit* target);

        static void DriveControlledFelhunter(
            PlayerbotAI* ai,
            Unit* charmer,
            Creature* krosh,
            Creature* maulgar);

        static uint32 ReleaseAllControlledFelhunters(PlayerbotAI* ai);
        static EncounterOverrideResult HandlePostKillFelhunterCleanup(PlayerbotAI* ai);
        static Unit* SelectKillOrderTarget(
            Creature* blindeye,
            Creature* olm,
            Creature* kiggler,
            Creature* krosh,
            Creature* maulgar);

        static bool IsRaidHealer(uint32 lowGuid);
        static bool IsMeleeDps(uint32 lowGuid);
        static bool IsBlindeyeHealing(Creature* blindeye);

        static EncounterOverrideResult HandleKroshMage(
            PlayerbotAI* ai, Creature* krosh);

        static EncounterOverrideResult HandleOlmWarlock(
            PlayerbotAI* ai,
            Creature* olm,
            Creature* uncontrolledFelhunter);

        static EncounterOverrideResult HandleBlindeyeInterrupt(
            PlayerbotAI* ai, Creature* blindeye);
    };
}
