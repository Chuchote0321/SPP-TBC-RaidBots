#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Internal.h"

#include "playerbot/PlayerbotAI.h"

#include "MotionGenerators/PathFinder.h"
#include "Util/Timer.h"

#include <algorithm>
#include <cmath>
#include <map>

using namespace ai;
using namespace ai::karazhan_phase1_detail;

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float EPSILON = 0.001f;
    constexpr float PATH_LIMIT = 160.0f;
    constexpr float MOVE_STEP = 5.0f;
    constexpr uint32 MOVE_REISSUE_MS = 900;

    std::map<Map*, std::map<uint32, uint32> > s_lastMoveMs;

    bool IsUsableGroupMember(PlayerbotAI* ai, Player* player)
    {
        return ai && ai->GetBot() && player && player->IsAlive() &&
               player->GetMap() == ai->GetBot()->GetMap();
    }

    bool PathAccepts(Player* actor, Slot const& slot)
    {
        if (!actor || !slot.valid)
            return false;

        PathFinder path(actor);
        path.setPathLengthLimit(PATH_LIMIT);
        path.calculate(slot.x, slot.y, slot.z, false, false);

        const uint32 type = static_cast<uint32>(path.getPathType());
        if (type & static_cast<uint32>(PATHFIND_NOPATH))
            return false;
        if (type & static_cast<uint32>(PATHFIND_SHORTCUT))
            return false;
        if (type & static_cast<uint32>(PATHFIND_INCOMPLETE))
            return false;
        if (type & static_cast<uint32>(PATHFIND_SHORT))
            return false;

        return type &
            (static_cast<uint32>(PATHFIND_NORMAL) |
             static_cast<uint32>(PATHFIND_NOT_USING_PATH));
    }

    bool AtSlot(Player* actor, Slot const& slot, float tolerance)
    {
        if (!actor || !slot.valid)
            return false;

        const float dx = actor->GetPositionX() - slot.x;
        const float dy = actor->GetPositionY() - slot.y;
        const float dz = std::fabs(actor->GetPositionZ() - slot.z);
        return dx * dx + dy * dy <= tolerance * tolerance &&
               dz <= 4.0f;
    }
}

float ai::karazhan_phase1_detail::Length(Vec2 const& value)
{
    return std::sqrt(value.x * value.x + value.y * value.y);
}

Vec2 ai::karazhan_phase1_detail::Normalize(Vec2 value)
{
    const float length = Length(value);
    if (length < EPSILON)
        return Vec2(1.0f, 0.0f);

    return value * (1.0f / length);
}

Vec2 ai::karazhan_phase1_detail::Perpendicular(
    Vec2 const& value)
{
    return Vec2(-value.y, value.x);
}

Vec2 ai::karazhan_phase1_detail::Position2(
    WorldObject const* object)
{
    return object
        ? Vec2(object->GetPositionX(), object->GetPositionY())
        : Vec2();
}

std::vector<Player*> ai::karazhan_phase1_detail::SortedGroup(
    PlayerbotAI* ai)
{
    std::vector<Player*> result;
    if (!ai || !ai->GetBot())
        return result;

    for (Player* player : ai->GetPlayersInGroup())
    {
        if (IsUsableGroupMember(ai, player))
            result.push_back(player);
    }

    std::sort(
        result.begin(),
        result.end(),
        [](Player* lhs, Player* rhs)
        {
            return lhs->GetObjectGuid().GetCounter() <
                   rhs->GetObjectGuid().GetCounter();
        });

    return result;
}

std::vector<Player*> ai::karazhan_phase1_detail::SortedTanks(
    PlayerbotAI* ai)
{
    std::vector<Player*> tanks;
    for (Player* player : SortedGroup(ai))
    {
        if (PlayerbotAI::IsTank(player, false))
            tanks.push_back(player);
    }

    std::stable_sort(
        tanks.begin(),
        tanks.end(),
        [](Player* lhs, Player* rhs)
        {
            if (lhs->GetMaxHealth() != rhs->GetMaxHealth())
                return lhs->GetMaxHealth() > rhs->GetMaxHealth();

            return lhs->GetObjectGuid().GetCounter() <
                   rhs->GetObjectGuid().GetCounter();
        });

    return tanks;
}

Vec2 ai::karazhan_phase1_detail::RaidCentroid(PlayerbotAI* ai)
{
    Vec2 center;
    uint32 count = 0;

    for (Player* player : SortedGroup(ai))
    {
        center = center + Position2(player);
        ++count;
    }

    return count
        ? center * (1.0f / static_cast<float>(count))
        : Position2(ai ? ai->GetBot() : nullptr);
}

Slot ai::karazhan_phase1_detail::RespawnCenter(Unit* target)
{
    Creature* creature = dynamic_cast<Creature*>(target);
    if (!creature)
    {
        return target
            ? Slot(
                  target->GetPositionX(),
                  target->GetPositionY(),
                  target->GetPositionZ())
            : Slot();
    }

    float x = creature->GetPositionX();
    float y = creature->GetPositionY();
    float z = creature->GetPositionZ();
    creature->GetRespawnCoord(x, y, z);
    return Slot(x, y, z);
}

bool ai::karazhan_phase1_detail::CandidateValid(
    Player* actor,
    Unit* target,
    Slot& slot,
    float minimumTargetDistance,
    float maximumTargetDistance)
{
    if (!actor || !slot.valid)
        return false;

    actor->UpdateAllowedPositionZ(slot.x, slot.y, slot.z);
    if (!actor->IsWithinLOS(slot.x, slot.y, slot.z))
        return false;

    if (target)
    {
        if (!target->IsWithinLOS(slot.x, slot.y, slot.z))
            return false;

        const float distance = target->GetDistance2d(slot.x, slot.y);
        if (minimumTargetDistance > 0.0f &&
            distance < minimumTargetDistance)
        {
            return false;
        }

        if (maximumTargetDistance > 0.0f &&
            distance > maximumTargetDistance)
        {
            return false;
        }

        slot.orientation = std::atan2(
            target->GetPositionY() - slot.y,
            target->GetPositionX() - slot.x);
    }

    return PathAccepts(actor, slot);
}

bool ai::karazhan_phase1_detail::MoveToward(
    PlayerbotAI* ai,
    Slot const& finalSlot,
    float tolerance,
    uint32 moveId)
{
    if (!ai || !ai->GetBot() || !finalSlot.valid)
        return false;

    Player* bot = ai->GetBot();
    if (AtSlot(bot, finalSlot, tolerance))
        return false;

    Map* map = bot->GetMap();
    const uint32 guid = bot->GetObjectGuid().GetCounter();
    const uint32 now = WorldTimer::getMSTime();
    const uint32 last = s_lastMoveMs[map][guid];

    if (last && now - last < MOVE_REISSUE_MS &&
        bot->GetMotionMaster()->GetCurrentMovementGeneratorType() ==
            POINT_MOTION_TYPE)
    {
        return true;
    }

    const float dx = finalSlot.x - bot->GetPositionX();
    const float dy = finalSlot.y - bot->GetPositionY();
    const float distance = std::sqrt(dx * dx + dy * dy);

    float x = finalSlot.x;
    float y = finalSlot.y;
    float z = finalSlot.z;

    if (distance > MOVE_STEP)
    {
        x = bot->GetPositionX() + dx / distance * MOVE_STEP;
        y = bot->GetPositionY() + dy / distance * MOVE_STEP;
        z = bot->GetPositionZ();
    }

    bot->UpdateAllowedPositionZ(x, y, z);
    Slot step(x, y, z, finalSlot.orientation);
    if (!bot->IsWithinLOS(step.x, step.y, step.z) ||
        !PathAccepts(bot, step))
    {
        return false;
    }

    bot->GetMotionMaster()->MovePoint(
        moveId,
        step.x,
        step.y,
        step.z,
        FORCED_MOVEMENT_RUN,
        true);

    s_lastMoveMs[map][guid] = now;
    return true;
}

Slot ai::karazhan_phase1_detail::FarSideTankSlot(
    PlayerbotAI* ai,
    Unit* target,
    Slot const& center,
    float offset)
{
    if (!ai || !target || !center.valid)
        return Slot();

    Vec2 entrance = Normalize(
        RaidCentroid(ai) - Vec2(center.x, center.y));
    const Vec2 side = Perpendicular(entrance);
    const Slot ideal(
        center.x - entrance.x * offset,
        center.y - entrance.y * offset,
        center.z);

    static const float sideOffsets[] =
        { 0.0f, 2.0f, -2.0f, 4.0f, -4.0f };
    static const float forwardOffsets[] =
        { 0.0f, 2.0f, -2.0f };

    for (float forward : forwardOffsets)
    {
        for (float lateral : sideOffsets)
        {
            Slot candidate = ideal;
            candidate.x +=
                entrance.x * forward + side.x * lateral;
            candidate.y +=
                entrance.y * forward + side.y * lateral;

            if (CandidateValid(
                    ai->GetBot(), target, candidate, 0.0f, 0.0f))
            {
                return candidate;
            }
        }
    }

    return Slot();
}

Slot ai::karazhan_phase1_detail::BehindTargetSlot(
    PlayerbotAI* ai,
    Unit* target,
    float distance)
{
    if (!ai || !target)
        return Slot();

    const float rear = target->GetOrientation() + PI;
    static const float angleOffsets[] =
        { 0.0f, 0.15f, -0.15f, 0.30f, -0.30f };

    for (float offset : angleOffsets)
    {
        const float angle = rear + offset;
        Slot candidate(
            target->GetPositionX() + std::cos(angle) * distance,
            target->GetPositionY() + std::sin(angle) * distance,
            target->GetPositionZ());

        if (CandidateValid(
                ai->GetBot(), target, candidate, 1.0f, distance + 3.0f))
        {
            return candidate;
        }
    }

    return Slot();
}

uint32 ai::karazhan_phase1_detail::RangedOrdinal(
    PlayerbotAI* ai,
    Player* actor)
{
    std::vector<uint32> guids;
    for (Player* player : SortedGroup(ai))
    {
        if (PlayerbotAI::IsHeal(player, false) ||
            ai->IsRanged(player, false))
        {
            guids.push_back(player->GetObjectGuid().GetCounter());
        }
    }

    const uint32 self = actor->GetObjectGuid().GetCounter();
    auto itr = std::find(guids.begin(), guids.end(), self);
    return itr == guids.end()
        ? 0
        : static_cast<uint32>(std::distance(guids.begin(), itr));
}

void ai::karazhan_phase1_detail::ResetMovement(PlayerbotAI* ai)
{
    if (ai && ai->GetBot() && ai->GetBot()->GetMap())
        s_lastMoveMs.erase(ai->GetBot()->GetMap());
}
