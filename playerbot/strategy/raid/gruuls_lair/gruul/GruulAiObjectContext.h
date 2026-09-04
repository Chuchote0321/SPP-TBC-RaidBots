#pragma once

#include "playerbot/strategy/NamedObjectContext.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulStrategy.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulTriggers.h"

namespace ai
{
    class GruulsLairStrategyContext final
        : public NamedObjectContext<Strategy>
    {
    public:
        GruulsLairStrategyContext()
        {
            creators["gruul's lair"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulsLairStrategy(ai);
                };
        }
    };

    class GruulsLairActionContext final
        : public NamedObjectContext<Action>
    {
    public:
        GruulsLairActionContext()
        {
            creators["enable gruul's lair strategy"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulsLairEnableStrategyAction(ai);
                };

            creators["disable gruul's lair strategy"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulsLairDisableStrategyAction(ai);
                };

            creators["gruul tank position"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulTankPositionAction(ai);
                };

            creators["gruul maintain ranged spread"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulRangedSpreadAction(ai);
                };

            creators["gruul shatter spread"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulShatterSpreadAction(ai);
                };

            creators["gruul reset encounter runtime"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulEncounterResetAction(ai);
                };
        }
    };

    class GruulsLairTriggerContext final
        : public NamedObjectContext<Trigger>
    {
    public:
        GruulsLairTriggerContext()
        {
            creators["enter gruul's lair"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulsLairEnterDungeonTrigger(ai);
                };

            creators["leave gruul's lair"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulsLairLeaveDungeonTrigger(ai);
                };

            creators["gruul tank positioning"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulTankPositionTrigger(ai);
                };

            creators["gruul ranged spread"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulRangedSpreadTrigger(ai);
                };

            creators["gruul incoming shatter"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulIncomingShatterTrigger(ai);
                };

            creators["gruul encounter reset"] =
                [](PlayerbotAI* ai)
                {
                    return new GruulEncounterResetTrigger(ai);
                };
        }
    };
}
