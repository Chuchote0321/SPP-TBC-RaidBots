#pragma once

#include <functional>
#include <string>

#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/NamedObjectContext.h"
#include "playerbot/strategy/Strategy.h"
#include "playerbot/strategy/Trigger.h"

namespace ai
{
    class RaidStrategyContext : public NamedObjectContext<Strategy>
    {
    public:
        using Creator = std::function<Strategy*(PlayerbotAI*)>;

        RaidStrategyContext();
        void Register(const std::string& name, Creator creator);
    };

    class RaidActionContext : public NamedObjectContext<Action>
    {
    public:
        using Creator = std::function<Action*(PlayerbotAI*)>;

        RaidActionContext();
        void Register(const std::string& name, Creator creator);
    };

    class RaidTriggerContext : public NamedObjectContext<Trigger>
    {
    public:
        using Creator = std::function<Trigger*(PlayerbotAI*)>;

        RaidTriggerContext();
        void Register(const std::string& name, Creator creator);
    };
}
