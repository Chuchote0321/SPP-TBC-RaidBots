#pragma once

#include "playerbot/strategy/Trigger.h"

namespace ai
{
    class KarazhanAttumenPhaseOneTrigger : public Trigger
    {
    public:
        KarazhanAttumenPhaseOneTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan attumen phase one", 1) {}

        bool IsActive() override;
    };

    class KarazhanAttumenPhaseTwoTrigger : public Trigger
    {
    public:
        KarazhanAttumenPhaseTwoTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan attumen phase two", 1) {}

        bool IsActive() override;
    };

    class KarazhanAttumenTransitionTrigger : public Trigger
    {
    public:
        KarazhanAttumenTransitionTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan attumen transition", 1) {}

        bool IsActive() override;
    };

    class KarazhanMoroesGuestPriorityTrigger : public Trigger
    {
    public:
        KarazhanMoroesGuestPriorityTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan moroes guest priority", 1) {}

        bool IsActive() override;
    };

    class KarazhanMaidenTankPositionTrigger : public Trigger
    {
    public:
        KarazhanMaidenTankPositionTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan maiden tank position", 1) {}

        bool IsActive() override;
    };

    class KarazhanMaidenRangedPositionTrigger : public Trigger
    {
    public:
        KarazhanMaidenRangedPositionTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan maiden ranged position", 1) {}

        bool IsActive() override;
    };

    class KarazhanMaidenGroundingTotemTrigger : public Trigger
    {
    public:
        KarazhanMaidenGroundingTotemTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan maiden grounding totem", 1) {}

        bool IsActive() override;
    };

    class KarazhanPhase1ResetTrigger : public Trigger
    {
    public:
        KarazhanPhase1ResetTrigger(PlayerbotAI* ai)
            : Trigger(ai, "karazhan phase one reset", 2) {}

        bool IsActive() override;
    };
}
