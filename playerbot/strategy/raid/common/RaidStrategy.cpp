#include "playerbot/playerbot.h"
#include "RaidStrategy.h"

using namespace ai;

RaidStrategy::RaidStrategy(PlayerbotAI* ai) : Strategy(ai)
{
}

NextAction** RaidStrategy::GetDefaultCombatActions()
{
    return GetRaidDefaultActions();
}

void RaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    InitRaidTriggers(triggers);
}

void RaidStrategy::InitCombatMultipliers(
    std::list<Multiplier*>& multipliers)
{
    InitRaidMultipliers(multipliers);
}
