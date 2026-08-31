#pragma once

#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class RaidStrategy : public Strategy
    {
    public:
        explicit RaidStrategy(PlayerbotAI* ai);

        int GetType() override
        {
            return STRATEGY_TYPE_COMBAT;
        }

    protected:
        NextAction** GetDefaultCombatActions() final override;
        void InitCombatTriggers(
            std::list<TriggerNode*>& triggers) final override;
        void InitCombatMultipliers(
            std::list<Multiplier*>& multipliers) final override;

        virtual NextAction** GetRaidDefaultActions()
        {
            return nullptr;
        }

        virtual void InitRaidTriggers(
            std::list<TriggerNode*>& triggers)
        {
        }

        virtual void InitRaidMultipliers(
            std::list<Multiplier*>& multipliers)
        {
        }
    };
}
