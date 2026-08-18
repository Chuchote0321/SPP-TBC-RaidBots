#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class PlayerbotAI;

namespace ai
{

    class GruulsLairTactics
    {
    public:
        static EncounterOverrideResult Update(PlayerbotAI* ai);

    private:
        GruulsLairTactics() = delete;
    };
}
