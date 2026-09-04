#pragma once

#include "playerbot/strategy/Multiplier.h"

namespace ai
{
    class GruulDelayBloodlustMultiplier : public Multiplier
    {
    public:
        GruulDelayBloodlustMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "gruul delay bloodlust") {}

        float GetValue(Action* action) override;
    };

    class GruulControlMainTankMovementMultiplier : public Multiplier
    {
    public:
        GruulControlMainTankMovementMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "gruul control main tank movement") {}

        float GetValue(Action* action) override;
    };

    class GruulShatterMovementMultiplier : public Multiplier
    {
    public:
        GruulShatterMovementMultiplier(PlayerbotAI* ai)
            : Multiplier(ai, "gruul shatter movement lock") {}

        float GetValue(Action* action) override;
    };
}
