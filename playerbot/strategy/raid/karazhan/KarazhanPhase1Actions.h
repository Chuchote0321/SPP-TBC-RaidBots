#pragma once

#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/actions/MovementActions.h"

namespace ai
{
    class KarazhanAttumenPhaseOneAction : public MovementAction
    {
    public:
        KarazhanAttumenPhaseOneAction(PlayerbotAI* ai)
            : MovementAction(ai, "karazhan handle attumen phase one") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class KarazhanAttumenPhaseTwoAction : public MovementAction
    {
    public:
        KarazhanAttumenPhaseTwoAction(PlayerbotAI* ai)
            : MovementAction(ai, "karazhan handle attumen phase two") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class KarazhanAttumenTransitionAction : public Action
    {
    public:
        KarazhanAttumenTransitionAction(PlayerbotAI* ai)
            : Action(ai, "karazhan observe attumen transition") {}

        bool Execute(Event& event) override;
    };

    class KarazhanMoroesGuestPriorityAction : public Action
    {
    public:
        KarazhanMoroesGuestPriorityAction(PlayerbotAI* ai)
            : Action(ai, "karazhan prioritize moroes guest") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class KarazhanMaidenTankPositionAction : public MovementAction
    {
    public:
        KarazhanMaidenTankPositionAction(PlayerbotAI* ai)
            : MovementAction(ai, "karazhan position maiden tank") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class KarazhanMaidenRangedPositionAction : public MovementAction
    {
    public:
        KarazhanMaidenRangedPositionAction(PlayerbotAI* ai)
            : MovementAction(ai, "karazhan position maiden ranged") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class KarazhanMaidenGroundingTotemAction : public Action
    {
    public:
        KarazhanMaidenGroundingTotemAction(PlayerbotAI* ai)
            : Action(ai, "karazhan cast maiden grounding totem") {}

        bool Execute(Event& event) override;
        bool isUseful() override;
    };

    class KarazhanPhase1ResetAction : public Action
    {
    public:
        KarazhanPhase1ResetAction(PlayerbotAI* ai)
            : Action(ai, "karazhan reset phase one runtime") {}

        bool Execute(Event& event) override;
    };
}
