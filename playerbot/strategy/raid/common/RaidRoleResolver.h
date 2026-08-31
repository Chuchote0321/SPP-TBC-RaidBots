#pragma once

#include "playerbot/PlayerbotAI.h"

#include <functional>
#include <map>
#include <vector>

namespace ai
{
    enum class RaidRole : uint8
    {
        MainTank = 0,
        AssistTank,
        MageTank,
        MoonkinTank,
        Healer,
        RangedDps,
        MeleeDps,
        Hunter,
        Warlock
    };

    struct RaidRoleSlot
    {
        RaidRoleSlot(RaidRole role, uint8 index)
            : role(role), index(index)
        {
        }

        bool operator<(const RaidRoleSlot& other) const
        {
            if (role != other.role)
            {
                return static_cast<uint8>(role) <
                    static_cast<uint8>(other.role);
            }

            return index < other.index;
        }

        RaidRole role;
        uint8 index;
    };

    class RaidRoleResolver : public PlayerbotAIAware
    {
    public:
        explicit RaidRoleResolver(PlayerbotAI* ai);

        void SetOverride(
            RaidRole role,
            ObjectGuid playerGuid,
            uint8 index = 0);

        void ClearOverride(
            RaidRole role,
            uint8 index = 0);

        void ClearOverrides();

        Player* Resolve(
            RaidRole role,
            uint8 index = 0) const;

        bool IsAssigned(
            Player* player,
            RaidRole role,
            uint8 index = 0) const;

        std::vector<Player*> GetRoster() const;

    private:
        Player* ResolveOverride(
            RaidRole role,
            uint8 index) const;

        Player* ResolveDefault(
            RaidRole role,
            uint8 index) const;

        Player* ResolveMainTank() const;
        Player* ResolveAssistTank(uint8 index) const;

        Player* SelectNth(
            const std::vector<Player*>& roster,
            const std::function<bool(Player*)>& predicate,
            uint8 index,
            Player* excluded = nullptr) const;

        Player* SelectHighestHealth(
            const std::vector<Player*>& roster,
            const std::function<bool(Player*)>& predicate) const;

        std::map<RaidRoleSlot, ObjectGuid> overrides;
    };
}
