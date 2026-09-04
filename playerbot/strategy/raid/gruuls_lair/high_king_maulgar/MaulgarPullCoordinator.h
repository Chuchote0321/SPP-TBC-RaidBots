#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class PlayerbotAI;

namespace ai
{
    // Compatibility API for downstream branches that still include
    // MaulgarPullCoordinator. The active preparation, positioning and
    // synchronized opening are owned by MaulgarPullCommandController.
    // No fixed coordinate, pull state or movement is retained here.
    class MaulgarPullCoordinator
    {
    public:
        static EncounterOverrideResult UpdatePrePull(PlayerbotAI* ai);
        static EncounterOverrideResult UpdateOpening(PlayerbotAI* ai);
        static void Reset(PlayerbotAI* ai);
        static bool IsConfigured();

    private:
        MaulgarPullCoordinator() = delete;
    };
}
