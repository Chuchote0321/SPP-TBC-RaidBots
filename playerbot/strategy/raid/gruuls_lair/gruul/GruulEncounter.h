#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class Creature;
class Unit;
class PlayerbotAI;

namespace ai
{
    class GruulEncounter
    {
    public:
        static EncounterOverrideResult Update(PlayerbotAI* ai);

    private:
        static Creature* FindGruul(PlayerbotAI* ai);
        static void SetEncounterTarget(PlayerbotAI* ai, Unit* target);
        static bool IsRaidHealer(uint32 lowGuid);
    };
}
