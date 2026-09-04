#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class Player;
class PlayerbotAI;

namespace ai
{
    enum class MaulgarPullCommandPhase : uint8_t
    {
        Idle = 0,
        Preparing,
        Armed,
        PullRequested,
        InProgress,
        Complete
    };

    // Explicit two-stage gate for High King Maulgar.
    // Player commands:
    //   /ra raid prepare maulgar
    //   /ra raid pull maulgar
    class MaulgarPullCommandController
    {
    public:
        static bool RequestPrepare(PlayerbotAI* ai, Player* requester);
        static bool RequestPull(PlayerbotAI* ai, Player* requester);

        static EncounterOverrideResult Update(PlayerbotAI* ai);
        static bool AllowEncounterDispatch(PlayerbotAI* ai);
        static void Reset(PlayerbotAI* ai);

        static MaulgarPullCommandPhase GetPhase(PlayerbotAI* ai);
        static const char* PhaseName(MaulgarPullCommandPhase phase);

    private:
        MaulgarPullCommandController() = delete;
    };
}
