#pragma once

#include "playerbot/PlayerbotAI.h"

#include <string>
#include <vector>

namespace ai
{
    class RaidTargetResolver : public PlayerbotAIAware
    {
    public:
        explicit RaidTargetResolver(PlayerbotAI* ai);

        Unit* Resolve(
            ObjectGuid guid,
            bool requireAlive = true) const;

        Unit* FindByEntry(
            uint32 entry,
            bool requireAlive = true) const;

        std::vector<Unit*> FindAllByEntry(
            uint32 entry,
            bool requireAlive = true) const;

        // Compatibility fallback only. Production encounter logic should
        // resolve targets by GUID or creature entry.
        Unit* FindByNameFallback(
            const std::string& name,
            bool requireAlive = true) const;

        std::vector<Unit*> CollectCandidates(
            bool requireAlive = true) const;

        bool IsUsable(
            Unit* target,
            bool requireAlive = true) const;

    private:
        void AppendGuidValue(
            const std::string& valueName,
            std::vector<Unit*>& result,
            bool requireAlive) const;

        void AppendUnitValue(
            const std::string& valueName,
            std::vector<Unit*>& result,
            bool requireAlive) const;

        static bool EqualsIgnoreCase(
            const std::string& left,
            const std::string& right);
    };
}
