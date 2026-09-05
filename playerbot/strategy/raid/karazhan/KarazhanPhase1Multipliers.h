#pragma once

#include "playerbot/strategy/Multiplier.h"

namespace ai
{
    class KarazhanAttumenTargetingMultiplier : public Multiplier
    {
    public:
        KarazhanAttumenTargetingMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "karazhan attumen targeting control") {}

        float GetValue(Action* action) override;
    };

    class KarazhanAttumenStackMultiplier : public Multiplier
    {
    public:
        KarazhanAttumenStackMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "karazhan attumen stack control") {}

        float GetValue(Action* action) override;
    };

    class KarazhanAttumenDpsWaitMultiplier : public Multiplier
    {
    public:
        KarazhanAttumenDpsWaitMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "karazhan attumen dps wait") {}

        float GetValue(Action* action) override;
    };

    class KarazhanMaidenFormationMultiplier : public Multiplier
    {
    public:
        KarazhanMaidenFormationMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "karazhan maiden formation control") {}

        float GetValue(Action* action) override;
    };

    class KarazhanMaidenGroundingTotemMultiplier : public Multiplier
    {
    public:
        KarazhanMaidenGroundingTotemMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "karazhan maiden grounding totem control") {}

        float GetValue(Action* action) override;
    };
}
