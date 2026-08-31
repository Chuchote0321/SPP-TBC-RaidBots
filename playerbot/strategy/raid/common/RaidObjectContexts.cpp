#include "playerbot/playerbot.h"
#include "RaidObjectContexts.h"

#include <utility>

using namespace ai;

RaidStrategyContext::RaidStrategyContext()
    : NamedObjectContext<Strategy>(false, false)
{
}

void RaidStrategyContext::Register(
    const std::string& name,
    Creator creator)
{
    if (!name.empty() && creator)
    {
        Erase(name);
        creators[name] = std::move(creator);
    }
}

RaidActionContext::RaidActionContext()
    : NamedObjectContext<Action>(false, false)
{
}

void RaidActionContext::Register(
    const std::string& name,
    Creator creator)
{
    if (!name.empty() && creator)
    {
        Erase(name);
        creators[name] = std::move(creator);
    }
}

RaidTriggerContext::RaidTriggerContext()
    : NamedObjectContext<Trigger>(false, false)
{
}

void RaidTriggerContext::Register(
    const std::string& name,
    Creator creator)
{
    if (!name.empty() && creator)
    {
        Erase(name);
        creators[name] = std::move(creator);
    }
}
