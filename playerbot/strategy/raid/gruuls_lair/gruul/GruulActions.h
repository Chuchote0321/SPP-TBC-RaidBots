#pragma once

#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/actions/ChangeStrategyAction.h"
#include "playerbot/strategy/actions/MovementActions.h"

namespace ai
{
    class GruulsLairEnableStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GruulsLairEnableStrategyAction(PlayerbotAI* ai)
            : ChangeAllStrategyAction(
                  ai,
                  "enable gruul's lair strategy",
                  "+gruul's lair") {}
    };

    class GruulsLairDisableStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GruulsLairDisableStrategyAction(PlayerbotAI* ai)
            : ChangeAllStrategyAction(
                  ai,
                  "disable gruul's lair strategy",
                  "-gruul's lair") {}
    };

    class GruulTankPositionAction : public MovementAction
    {
    public:
        GruulTankPositionAction(PlayerbotAI* ai)
            : MovementAction(ai, "gruul tank position") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class GruulRangedSpreadAction : public MovementAction
    {
    public:
        GruulRangedSpreadAction(PlayerbotAI* ai)
            : MovementAction(ai, "gruul maintain ranged spread") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class GruulShatterSpreadAction : public MovementAction
    {
    public:
        GruulShatterSpreadAction(PlayerbotAI* ai)
            : MovementAction(ai, "gruul shatter spread") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class GruulEncounterResetAction : public Action
    {
    public:
        GruulEncounterResetAction(PlayerbotAI* ai)
            : Action(ai, "gruul reset encounter runtime") {}

        bool Execute(Event& event) override;
    };
}
