#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulStrategy.h"

#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulMultipliers.h"

using namespace ai;

void GruulsLairStrategy::InitCombatTriggers(
    std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "gruul incoming shatter",
        NextAction::array(
            0,
            new NextAction(
                "gruul shatter spread",
                ACTION_EMERGENCY + 8),
            NULL)));

    triggers.push_back(new TriggerNode(
        "gruul tank positioning",
        NextAction::array(
            0,
            new NextAction(
                "gruul tank position",
                ACTION_MOVE + 8),
            NULL)));

    triggers.push_back(new TriggerNode(
        "gruul ranged spread",
        NextAction::array(
            0,
            new NextAction(
                "gruul maintain ranged spread",
                ACTION_MOVE + 6),
            NULL)));
}

void GruulsLairStrategy::InitNonCombatTriggers(
    std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "gruul encounter reset",
        NextAction::array(
            0,
            new NextAction(
                "gruul reset encounter runtime",
                ACTION_IDLE),
            NULL)));
}

void GruulsLairStrategy::InitCombatMultipliers(
    std::list<Multiplier*>& multipliers)
{
    multipliers.push_back(
        new GruulDelayBloodlustMultiplier(ai));
    multipliers.push_back(
        new GruulControlMainTankMovementMultiplier(ai));
    multipliers.push_back(
        new GruulShatterMovementMultiplier(ai));
}
