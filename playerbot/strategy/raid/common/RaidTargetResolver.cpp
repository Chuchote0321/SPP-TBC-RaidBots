#include "playerbot/playerbot.h"
#include "RaidTargetResolver.h"

#include <algorithm>
#include <cctype>
#include <list>

using namespace ai;

namespace
{
    void AppendUnique(std::vector<Unit*>& result, Unit* target)
    {
        if (target &&
            std::find(result.begin(), result.end(), target) == result.end())
        {
            result.push_back(target);
        }
    }
}

RaidTargetResolver::RaidTargetResolver(PlayerbotAI* ai)
    : PlayerbotAIAware(ai)
{
}

bool RaidTargetResolver::IsUsable(
    Unit* target,
    bool requireAlive) const
{
    if (!ai || !target || !target->IsInWorld() || !ai->IsSafe(target))
    {
        return false;
    }

    return !requireAlive || target->IsAlive();
}

Unit* RaidTargetResolver::Resolve(
    ObjectGuid guid,
    bool requireAlive) const
{
    Unit* target = ai ? ai->GetUnit(guid) : nullptr;
    return IsUsable(target, requireAlive) ? target : nullptr;
}

void RaidTargetResolver::AppendGuidValue(
    const std::string& valueName,
    std::vector<Unit*>& result,
    bool requireAlive) const
{
    AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
    if (!context)
    {
        return;
    }

    Value<std::list<ObjectGuid>>* value =
        context->GetValue<std::list<ObjectGuid>>(valueName);
    if (!value)
    {
        return;
    }

    for (const ObjectGuid& guid : value->Get())
    {
        Unit* target = Resolve(guid, requireAlive);
        if (target)
        {
            AppendUnique(result, target);
        }
    }
}

void RaidTargetResolver::AppendUnitValue(
    const std::string& valueName,
    std::vector<Unit*>& result,
    bool requireAlive) const
{
    AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
    if (!context)
    {
        return;
    }

    Value<Unit*>* value = context->GetValue<Unit*>(valueName);
    if (!value)
    {
        return;
    }

    Unit* target = value->Get();
    if (IsUsable(target, requireAlive))
    {
        AppendUnique(result, target);
    }
}

std::vector<Unit*> RaidTargetResolver::CollectCandidates(
    bool requireAlive) const
{
    std::vector<Unit*> result;

    AppendUnitValue("current target", result, requireAlive);
    AppendUnitValue("rti target", result, requireAlive);
    AppendUnitValue("pull target", result, requireAlive);

    AppendGuidValue("attackers", result, requireAlive);
    AppendGuidValue("possible attack targets", result, requireAlive);
    AppendGuidValue("possible targets", result, requireAlive);
    AppendGuidValue("all targets", result, requireAlive);

    return result;
}

Unit* RaidTargetResolver::FindByEntry(
    uint32 entry,
    bool requireAlive) const
{
    for (Unit* target : CollectCandidates(requireAlive))
    {
        if (target->GetEntry() == entry)
        {
            return target;
        }
    }

    return nullptr;
}

std::vector<Unit*> RaidTargetResolver::FindAllByEntry(
    uint32 entry,
    bool requireAlive) const
{
    std::vector<Unit*> result;

    for (Unit* target : CollectCandidates(requireAlive))
    {
        if (target->GetEntry() == entry)
        {
            result.push_back(target);
        }
    }

    return result;
}

bool RaidTargetResolver::EqualsIgnoreCase(
    const std::string& left,
    const std::string& right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    return std::equal(
        left.begin(),
        left.end(),
        right.begin(),
        [](unsigned char a, unsigned char b)
        {
            return std::tolower(a) == std::tolower(b);
        });
}

Unit* RaidTargetResolver::FindByNameFallback(
    const std::string& name,
    bool requireAlive) const
{
    for (Unit* target : CollectCandidates(requireAlive))
    {
        if (EqualsIgnoreCase(target->GetName(), name))
        {
            return target;
        }
    }

    return nullptr;
}
