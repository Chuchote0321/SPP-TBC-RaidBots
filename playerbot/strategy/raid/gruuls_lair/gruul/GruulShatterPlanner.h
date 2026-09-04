#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class Creature;
class PlayerbotAI;

namespace ai
{
    // Shared, encounter-local Shatter positioning planner.
    //
    // Every bot scores candidate points against both live raid-member
    // positions and slots already reserved by other bots in the same map.
    // This is deliberately different from generic Flee(), which has no
    // pairwise raid-spacing objective.
    class GruulShatterPlanner
    {
    public:
        static EncounterOverrideResult Update(PlayerbotAI* ai, Creature* gruul);
        static void Reset(PlayerbotAI* ai);

        // Exposed for smoke-test/debug telemetry.
        static float CurrentMinimumSeparation(PlayerbotAI* ai);

    private:
        GruulShatterPlanner() = delete;
    };
}
