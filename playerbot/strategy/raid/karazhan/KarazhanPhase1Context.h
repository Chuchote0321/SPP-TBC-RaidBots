#pragma once

#include "playerbot/strategy/NamedObjectContext.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Actions.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Triggers.h"

namespace ai
{
    class KarazhanPhase1ActionContext final
        : public NamedObjectContext<Action>
    {
    public:
        KarazhanPhase1ActionContext()
        {
            creators["karazhan handle attumen phase one"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanAttumenPhaseOneAction(ai);
                };

            creators["karazhan handle attumen phase two"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanAttumenPhaseTwoAction(ai);
                };

            creators["karazhan observe attumen transition"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanAttumenTransitionAction(ai);
                };

            creators["karazhan prioritize moroes guest"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMoroesGuestPriorityAction(ai);
                };

            creators["karazhan position maiden tank"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMaidenTankPositionAction(ai);
                };

            creators["karazhan position maiden ranged"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMaidenRangedPositionAction(ai);
                };

            creators["karazhan cast maiden grounding totem"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMaidenGroundingTotemAction(ai);
                };

            creators["karazhan reset phase one runtime"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanPhase1ResetAction(ai);
                };
        }
    };

    class KarazhanPhase1TriggerContext final
        : public NamedObjectContext<Trigger>
    {
    public:
        KarazhanPhase1TriggerContext()
        {
            creators["karazhan attumen phase one"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanAttumenPhaseOneTrigger(ai);
                };

            creators["karazhan attumen phase two"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanAttumenPhaseTwoTrigger(ai);
                };

            creators["karazhan attumen transition"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanAttumenTransitionTrigger(ai);
                };

            creators["karazhan moroes guest priority"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMoroesGuestPriorityTrigger(ai);
                };

            creators["karazhan maiden tank position"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMaidenTankPositionTrigger(ai);
                };

            creators["karazhan maiden ranged position"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMaidenRangedPositionTrigger(ai);
                };

            creators["karazhan maiden grounding totem"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanMaidenGroundingTotemTrigger(ai);
                };

            creators["karazhan phase one reset"] =
                [](PlayerbotAI* ai)
                {
                    return new KarazhanPhase1ResetTrigger(ai);
                };
        }
    };
}
