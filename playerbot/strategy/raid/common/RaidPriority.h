#pragma once

#include "playerbot/strategy/Strategy.h"

namespace ai
{
    enum class RaidUrgency : uint8
    {
        Routine = 0,
        Positioning,
        Control,
        CriticalMovement,
        Emergency
    };

    float GetRaidRelevance(RaidUrgency urgency, float offset = 0.0f);
}
