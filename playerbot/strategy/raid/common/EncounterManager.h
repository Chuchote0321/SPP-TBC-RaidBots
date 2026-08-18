#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class PlayerbotAI;

namespace ai
{

    class EncounterManager
    {
    public:
        static EncounterManager& Instance();

        // Called exactly once immediately before Normal Rotation's
        // currentEngine->DoNextAction().
        EncounterOverrideResult Update(PlayerbotAI* ai);

    private:
        EncounterManager() = default;
        EncounterManager(const EncounterManager&) = delete;
        EncounterManager& operator=(const EncounterManager&) = delete;
    };
}
