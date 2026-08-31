#pragma once

#include "playerbot/PlayerbotAI.h"

#include <string>

namespace ai
{
    class RaidCoreFacade : public PlayerbotAIAware
    {
    public:
        explicit RaidCoreFacade(PlayerbotAI* ai);

        PlayerbotAI* GetAI() const
        {
            return ai;
        }

        Player* GetBot() const;
        AiObjectContext* GetContext() const;

        bool IsUsable(Unit* target, bool requireAlive = true) const;
        bool HasAura(
            Unit* target,
            uint32 spellId,
            bool checkOwner = false) const;

        bool CanCast(
            uint32 spellId,
            Unit* target,
            uint8 effectMask,
            bool checkHasSpell = true) const;

        bool Cast(
            uint32 spellId,
            Unit* target,
            bool waitForSpell = true) const;

        bool SelectTarget(Unit* target) const;
        bool Attack(Unit* target) const;

        bool ExecuteAction(
            const std::string& actionName,
            bool silent = true) const;

        void InterruptSpell(bool withMeleeAndAuto = true) const;
        void StopMoving() const;
    };
}
