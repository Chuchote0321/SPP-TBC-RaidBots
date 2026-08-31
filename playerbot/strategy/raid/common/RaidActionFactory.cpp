#include "playerbot/playerbot.h"
#include "RaidActionFactory.h"

#include <cstddef>

using namespace ai;

NextAction** ai::MakeRaidActionArray(
    std::initializer_list<RaidActionSpec> actions)
{
    NextAction** result = new NextAction*[actions.size() + 1];
    std::size_t index = 0;

    for (const RaidActionSpec& action : actions)
    {
        result[index++] = new NextAction(
            action.name,
            GetRaidRelevance(action.urgency, action.offset));
    }

    result[index] = nullptr;
    return result;
}

TriggerNode* ai::MakeRaidTriggerNode(
    const std::string& triggerName,
    std::initializer_list<RaidActionSpec> actions)
{
    return new TriggerNode(triggerName, MakeRaidActionArray(actions));
}

void ai::AddRaidTrigger(
    std::list<TriggerNode*>& triggers,
    const std::string& triggerName,
    std::initializer_list<RaidActionSpec> actions)
{
    triggers.push_back(MakeRaidTriggerNode(triggerName, actions));
}
