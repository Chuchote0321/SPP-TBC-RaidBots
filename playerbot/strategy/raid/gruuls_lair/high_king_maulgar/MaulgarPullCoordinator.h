#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class PlayerbotAI;

namespace ai
{

    class MaulgarPullCoordinator
    {
    public:
        // NOT_STARTED: move designated actors to absolute anchors, pre-cast
        // Misdirection for bot-Mage pulls, and hold the raid behind a pull barrier.
        static EncounterOverrideResult UpdatePrePull(PlayerbotAI* ai);

        // IN_PROGRESS: finish/consume the three Hunter Misdirection openers before
        // releasing them to the normal encounter kill order.
        static EncounterOverrideResult UpdateOpening(PlayerbotAI* ai);

        static void Reset(PlayerbotAI* ai);
        static bool IsConfigured();

    private:
        MaulgarPullCoordinator() = delete;
    };
}
