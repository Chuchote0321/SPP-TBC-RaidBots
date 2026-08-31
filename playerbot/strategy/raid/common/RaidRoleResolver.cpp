#include "playerbot/playerbot.h"
#include "RaidRoleResolver.h"

#include <algorithm>

using namespace ai;

RaidRoleResolver::RaidRoleResolver(PlayerbotAI* ai)
    : PlayerbotAIAware(ai)
{
}

void RaidRoleResolver::SetOverride(
    RaidRole role,
    ObjectGuid playerGuid,
    uint8 index)
{
    overrides[RaidRoleSlot(role, index)] = playerGuid;
}

void RaidRoleResolver::ClearOverride(
    RaidRole role,
    uint8 index)
{
    overrides.erase(RaidRoleSlot(role, index));
}

void RaidRoleResolver::ClearOverrides()
{
    overrides.clear();
}

std::vector<Player*> RaidRoleResolver::GetRoster() const
{
    std::vector<Player*> roster =
        ai ? ai->GetPlayersInGroup() : std::vector<Player*>();

    if (roster.empty() && ai && ai->GetBot())
    {
        roster.push_back(ai->GetBot());
    }

    roster.erase(
        std::remove(
            roster.begin(),
            roster.end(),
            static_cast<Player*>(nullptr)),
        roster.end());

    return roster;
}

Player* RaidRoleResolver::ResolveOverride(
    RaidRole role,
    uint8 index) const
{
    auto found = overrides.find(RaidRoleSlot(role, index));
    if (found == overrides.end() || !ai)
    {
        return nullptr;
    }

    Unit* unit = ai->GetUnit(found->second);
    return unit && unit->IsPlayer() && ai->IsSafe(unit) ?
        static_cast<Player*>(unit) : nullptr;
}

Player* RaidRoleResolver::SelectNth(
    const std::vector<Player*>& roster,
    const std::function<bool(Player*)>& predicate,
    uint8 index,
    Player* excluded) const
{
    uint8 current = 0;

    for (Player* player : roster)
    {
        if (!player || player == excluded || !predicate(player))
        {
            continue;
        }

        if (current++ == index)
        {
            return player;
        }
    }

    return nullptr;
}

Player* RaidRoleResolver::SelectHighestHealth(
    const std::vector<Player*>& roster,
    const std::function<bool(Player*)>& predicate) const
{
    Player* selected = nullptr;

    for (Player* player : roster)
    {
        if (!player || !predicate(player))
        {
            continue;
        }

        if (!selected || player->GetMaxHealth() > selected->GetMaxHealth())
        {
            selected = player;
        }
    }

    return selected;
}

Player* RaidRoleResolver::ResolveMainTank() const
{
    if (!ai)
    {
        return nullptr;
    }

    Player* groupMaster = ai->GetGroupMaster();
    if (groupMaster && PlayerbotAI::IsTank(groupMaster))
    {
        return groupMaster;
    }

    const std::vector<Player*> roster = GetRoster();
    return SelectNth(
        roster,
        [](Player* player)
        {
            return PlayerbotAI::IsTank(player);
        },
        0);
}

Player* RaidRoleResolver::ResolveAssistTank(uint8 index) const
{
    const std::vector<Player*> roster = GetRoster();
    Player* mainTank = ResolveMainTank();

    return SelectNth(
        roster,
        [](Player* player)
        {
            return PlayerbotAI::IsTank(player);
        },
        index,
        mainTank);
}

Player* RaidRoleResolver::ResolveDefault(
    RaidRole role,
    uint8 index) const
{
    if (!ai)
    {
        return nullptr;
    }

    const std::vector<Player*> roster = GetRoster();

    switch (role)
    {
        case RaidRole::MainTank:
            return ResolveMainTank();

        case RaidRole::AssistTank:
            return ResolveAssistTank(index);

        case RaidRole::MageTank:
            return SelectHighestHealth(
                roster,
                [](Player* player)
                {
                    return player->getClass() == CLASS_MAGE;
                });

        case RaidRole::MoonkinTank:
            return SelectHighestHealth(
                roster,
                [this](Player* player)
                {
                    return player->getClass() == CLASS_DRUID &&
                        ai->IsRanged(player) &&
                        !PlayerbotAI::IsHeal(player);
                });

        case RaidRole::Healer:
            return SelectNth(
                roster,
                [](Player* player)
                {
                    return PlayerbotAI::IsHeal(player);
                },
                index);

        case RaidRole::RangedDps:
            return SelectNth(
                roster,
                [this](Player* player)
                {
                    return ai->IsRanged(player) &&
                        !PlayerbotAI::IsHeal(player) &&
                        !PlayerbotAI::IsTank(player);
                },
                index);

        case RaidRole::MeleeDps:
            return SelectNth(
                roster,
                [this](Player* player)
                {
                    return ai->IsMelee(player) &&
                        !PlayerbotAI::IsHeal(player) &&
                        !PlayerbotAI::IsTank(player);
                },
                index);

        case RaidRole::Hunter:
            return SelectNth(
                roster,
                [](Player* player)
                {
                    return player->getClass() == CLASS_HUNTER;
                },
                index);

        case RaidRole::Warlock:
            return SelectNth(
                roster,
                [](Player* player)
                {
                    return player->getClass() == CLASS_WARLOCK;
                },
                index);
    }

    return nullptr;
}

Player* RaidRoleResolver::Resolve(
    RaidRole role,
    uint8 index) const
{
    if (Player* player = ResolveOverride(role, index))
    {
        return player;
    }

    return ResolveDefault(role, index);
}

bool RaidRoleResolver::IsAssigned(
    Player* player,
    RaidRole role,
    uint8 index) const
{
    Player* assigned = Resolve(role, index);
    return player &&
        assigned &&
        player->GetObjectGuid() == assigned->GetObjectGuid();
}
