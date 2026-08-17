#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

namespace ai
{
    class PlayerbotAI;

    class GruulsLairTactics
    {
    public:
        static EncounterOverrideResult Update(PlayerbotAI* ai);

    private:
        GruulsLairTactics() = delete;
    };
}
