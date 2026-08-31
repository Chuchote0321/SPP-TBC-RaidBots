#pragma once

#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/Trigger.h"
#include "RaidPriority.h"

#include <initializer_list>
#include <list>
#include <string>

namespace ai
{
    struct RaidActionSpec
    {
        RaidActionSpec(
            const std::string& name,
            RaidUrgency urgency,
            float offset = 0.0f)
            : name(name), urgency(urgency), offset(offset)
        {
        }

        std::string name;
        RaidUrgency urgency;
        float offset;
    };

    NextAction** MakeRaidActionArray(
        std::initializer_list<RaidActionSpec> actions);

    TriggerNode* MakeRaidTriggerNode(
        const std::string& triggerName,
        std::initializer_list<RaidActionSpec> actions);

    void AddRaidTrigger(
        std::list<TriggerNode*>& triggers,
        const std::string& triggerName,
        std::initializer_list<RaidActionSpec> actions);
}
