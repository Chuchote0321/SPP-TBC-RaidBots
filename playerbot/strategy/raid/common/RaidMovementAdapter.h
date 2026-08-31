#pragma once

#include <string>

#include "playerbot/strategy/actions/MovementActions.h"

namespace ai
{
    // Base class for encounter actions that need TBC movement semantics.
    class RaidMovementAdapter : public MovementAction
    {
    public:
        RaidMovementAdapter(
            PlayerbotAI* ai,
            const std::string& name);

    protected:
        bool MoveToRaidPosition(
            const WorldPosition& position,
            bool react = true,
            bool noPath = false,
            bool ignoreEnemyTargets = true);

        bool MoveNearRaidPosition(
            const WorldPosition& position,
            float distance);

        bool MoveToRaidUnit(
            Unit* target,
            float distance = 0.0f);

        bool MoveNearRaidUnit(
            Unit* target,
            float distance);

        bool FollowRaidUnit(
            Unit* target,
            float distance = 0.0f);

        bool FollowRaidUnit(
            Unit* target,
            float distance,
            float angle);

        bool ChaseRaidUnit(
            Unit* target,
            float distance = 0.0f,
            float angle = 0.0f);

        bool FleeFromRaidUnit(Unit* target);
    };
}
