#pragma once

#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class GruulsLairStrategy : public Strategy
    {
    public:
        GruulsLairStrategy(PlayerbotAI* ai) : Strategy(ai) {}

        std::string getName() override
        {
            return "gruul's lair";
        }

    private:
        void InitCombatTriggers(
            std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(
            std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(
            std::list<Multiplier*>& multipliers) override;
    };
}
