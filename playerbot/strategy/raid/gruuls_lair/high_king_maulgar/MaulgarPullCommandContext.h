#pragma once

#include "playerbot/strategy/NamedObjectContext.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandTriggers.h"

namespace ai
{
    class MaulgarPullCommandTriggerContext final
        : public NamedObjectContext<Trigger>
    {
    public:
        MaulgarPullCommandTriggerContext()
        {
            creators["raid prepare maulgar"] =
                [](PlayerbotAI* ai)
                {
                    return new MaulgarPrepareCommandTrigger(ai);
                };

            creators["raid pull maulgar"] =
                [](PlayerbotAI* ai)
                {
                    return new MaulgarPullCommandTrigger(ai);
                };
        }
    };
}
