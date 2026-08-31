#include "playerbot/playerbot.h"
#include "RaidPriority.h"

#include <algorithm>

using namespace ai;

float ai::GetRaidRelevance(RaidUrgency urgency, float offset)
{
    float relevance = ACTION_HIGH + 1.0f;

    switch (urgency)
    {
        case RaidUrgency::Routine:
            relevance = ACTION_HIGH + 1.0f;
            break;
        case RaidUrgency::Positioning:
            relevance = ACTION_MOVE + 1.0f;
            break;
        case RaidUrgency::Control:
            relevance = ACTION_DISPEL + 1.0f;
            break;
        case RaidUrgency::CriticalMovement:
            relevance = ACTION_CRITICAL_HEAL + 5.0f;
            break;
        case RaidUrgency::Emergency:
            relevance = ACTION_EMERGENCY + 5.0f;
            break;
    }

    relevance += offset;
    return std::max<float>(
        ACTION_IDLE,
        std::min<float>(relevance, ACTION_PASSTROUGH - 1.0f));
}
