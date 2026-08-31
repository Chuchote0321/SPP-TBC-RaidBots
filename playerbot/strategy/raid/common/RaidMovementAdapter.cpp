#include "playerbot/playerbot.h"
#include "RaidMovementAdapter.h"

using namespace ai;

RaidMovementAdapter::RaidMovementAdapter(
    PlayerbotAI* ai,
    const std::string& name)
    : MovementAction(ai, name)
{
}

bool RaidMovementAdapter::MoveToRaidPosition(
    const WorldPosition& position,
    bool react,
    bool noPath,
    bool ignoreEnemyTargets)
{
    return MoveTo(
        position,
        false,
        react,
        noPath,
        ignoreEnemyTargets);
}

bool RaidMovementAdapter::MoveNearRaidPosition(
    const WorldPosition& position,
    float distance)
{
    return MoveNear(
        position.getMapId(),
        position.getX(),
        position.getY(),
        position.getZ(),
        distance);
}

bool RaidMovementAdapter::MoveToRaidUnit(
    Unit* target,
    float distance)
{
    return target && MoveTo(target, distance);
}

bool RaidMovementAdapter::MoveNearRaidUnit(
    Unit* target,
    float distance)
{
    return target && MoveNear(target, distance);
}

bool RaidMovementAdapter::FollowRaidUnit(
    Unit* target,
    float distance)
{
    return target && Follow(target, distance);
}

bool RaidMovementAdapter::FollowRaidUnit(
    Unit* target,
    float distance,
    float angle)
{
    return target && Follow(target, distance, angle);
}

bool RaidMovementAdapter::ChaseRaidUnit(
    Unit* target,
    float distance,
    float angle)
{
    return target && ChaseTo(target, distance, angle);
}

bool RaidMovementAdapter::FleeFromRaidUnit(Unit* target)
{
    return target && Flee(target);
}
