#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/PlayerbotAI.h"

#include "Spells/Spell.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <vector>

using namespace ai;

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;

    // Shatter damage is distance dependent. The planner targets at least
    // 16 yards while accepting 13 yards as a practical minimum when the room
    // or current knockback geometry prevents a clean slot.
    constexpr float DESIRED_SEPARATION = 16.0f;
    constexpr float ACCEPTABLE_SEPARATION = 13.0f;

    constexpr float MOVE_TOLERANCE = 1.5f;
    constexpr float MIN_GRUUL_RADIUS = 9.0f;
    constexpr float MAX_GRUUL_RADIUS = 48.0f;

    constexpr uint32 SHATTER_MOVE_ID_BASE = 0x47525500; // "GRU\0"

    struct Vec2
    {
        float x;
        float y;

        Vec2() : x(0.0f), y(0.0f) {}
        Vec2(float px, float py) : x(px), y(py) {}
    };

    struct PlannedSlot
    {
        float x;
        float y;
        float z;
        float predictedMinimum;

        PlannedSlot()
            : x(0.0f), y(0.0f), z(0.0f), predictedMinimum(0.0f) {}

        PlannedSlot(float px, float py, float pz, float minimum)
            : x(px), y(py), z(pz), predictedMinimum(minimum) {}
    };

    using SlotMap = std::map<uint32, PlannedSlot>;
    std::map<Map*, SlotMap> s_shatterSlots;

    float Distance(Vec2 const& lhs, Vec2 const& rhs)
    {
        const float dx = lhs.x - rhs.x;
        const float dy = lhs.y - rhs.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    Vec2 Position2(WorldObject const* object)
    {
        return object
            ? Vec2(object->GetPositionX(), object->GetPositionY())
            : Vec2();
    }

    bool IsShatterSpell(Spell* spell)
    {
        if (!spell || !spell->m_spellInfo)
            return false;

        switch (spell->m_spellInfo->Id)
        {
            case EncounterConstants::SPELL_GRUUL_GROUND_SLAM:
            case EncounterConstants::SPELL_GRUUL_GROUND_SLAM_TRIGGER:
            case EncounterConstants::SPELL_GRUUL_GROUND_SLAM_DUMMY:
            case EncounterConstants::SPELL_GRUUL_SHATTER:
            case EncounterConstants::SPELL_GRUUL_SHATTER_EFFECT:
                return true;
            default:
                return false;
        }
    }

    bool IsShatterWindow(PlayerbotAI* ai, Creature* gruul)
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();

        // Keep the trigger compatible with both the upstream experimental
        // strategy and the current CMaNGOS ScriptDevAI spell sequence.
        if (bot->HasAura(EncounterConstants::SPELL_GRUUL_GROUND_SLAM) ||
            bot->HasAura(EncounterConstants::SPELL_GRUUL_GROUND_SLAM_TRIGGER) ||
            bot->HasAura(EncounterConstants::SPELL_GRUUL_GROUND_SLAM_DUMMY) ||
            bot->HasAura(EncounterConstants::SPELL_GRUUL_STONED))
        {
            return true;
        }

        return gruul && IsShatterSpell(
            gruul->GetCurrentSpell(CURRENT_GENERIC_SPELL));
    }

    std::vector<Player*> LiveRaidMembers(PlayerbotAI* ai)
    {
        std::vector<Player*> result;

        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return result;

        Map* map = ai->GetBot()->GetMap();
        const std::vector<Player*> players = ai->GetPlayersInGroup();

        for (Player* player : players)
        {
            if (!player || !player->IsAlive() || player->GetMap() != map)
                continue;

            result.push_back(player);
        }

        return result;
    }

    void PruneSlots(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return;

        Map* map = ai->GetBot()->GetMap();
        auto mapItr = s_shatterSlots.find(map);
        if (mapItr == s_shatterSlots.end())
            return;

        std::set<uint32> liveGuids;
        for (Player* player : LiveRaidMembers(ai))
            liveGuids.insert(player->GetObjectGuid().GetCounter());

        SlotMap& slots = mapItr->second;
        for (auto itr = slots.begin(); itr != slots.end(); )
        {
            if (liveGuids.find(itr->first) == liveGuids.end())
                itr = slots.erase(itr);
            else
                ++itr;
        }

        if (slots.empty())
            s_shatterSlots.erase(mapItr);
    }

    float MinimumDistanceToRaid(
        PlayerbotAI* ai,
        Vec2 const& candidate,
        uint32 selfGuid,
        bool includeReservedSlots)
    {
        float minimum = std::numeric_limits<float>::max();

        for (Player* player : LiveRaidMembers(ai))
        {
            if (player->GetObjectGuid().GetCounter() == selfGuid)
                continue;

            minimum = std::min(minimum, Distance(candidate, Position2(player)));
        }

        if (includeReservedSlots &&
            ai && ai->GetBot() && ai->GetBot()->GetMap())
        {
            auto mapItr = s_shatterSlots.find(ai->GetBot()->GetMap());
            if (mapItr != s_shatterSlots.end())
            {
                for (auto const& item : mapItr->second)
                {
                    if (item.first == selfGuid)
                        continue;

                    minimum = std::min(
                        minimum,
                        Distance(candidate, Vec2(item.second.x, item.second.y)));
                }
            }
        }

        // A one-player group has no Shatter collision partner.
        return minimum == std::numeric_limits<float>::max()
            ? 999.0f
            : minimum;
    }

    float CandidateScore(
        PlayerbotAI* ai,
        Creature* gruul,
        Vec2 const& current,
        Vec2 const& candidate,
        uint32 selfGuid,
        float& minimumOut)
    {
        minimumOut = MinimumDistanceToRaid(
            ai, candidate, selfGuid, true);

        float crowdingPenalty = 0.0f;

        for (Player* player : LiveRaidMembers(ai))
        {
            if (player->GetObjectGuid().GetCounter() == selfGuid)
                continue;

            const float distance = Distance(candidate, Position2(player));
            if (distance < DESIRED_SEPARATION)
            {
                const float deficit = DESIRED_SEPARATION - distance;
                crowdingPenalty += deficit * deficit;
            }
        }

        if (ai && ai->GetBot() && ai->GetBot()->GetMap())
        {
            auto mapItr = s_shatterSlots.find(ai->GetBot()->GetMap());
            if (mapItr != s_shatterSlots.end())
            {
                for (auto const& item : mapItr->second)
                {
                    if (item.first == selfGuid)
                        continue;

                    const float distance = Distance(
                        candidate,
                        Vec2(item.second.x, item.second.y));

                    if (distance < DESIRED_SEPARATION)
                    {
                        const float deficit = DESIRED_SEPARATION - distance;
                        crowdingPenalty += deficit * deficit;
                    }
                }
            }
        }

        const float travelDistance = Distance(current, candidate);
        const float gruulRadius = gruul
            ? Distance(candidate, Position2(gruul))
            : MIN_GRUUL_RADIUS;

        float arenaPenalty = 0.0f;
        if (gruulRadius < MIN_GRUUL_RADIUS)
            arenaPenalty += (MIN_GRUUL_RADIUS - gruulRadius) * 12.0f;
        if (gruulRadius > MAX_GRUUL_RADIUS)
            arenaPenalty += (gruulRadius - MAX_GRUUL_RADIUS) * 12.0f;

        // Max-min separation is the primary objective. The quadratic crowding
        // term prevents a candidate from looking good merely because one
        // neighbour is far away while another is still dangerously close.
        return std::min(minimumOut, DESIRED_SEPARATION + 8.0f) * 12.0f
             - crowdingPenalty * 1.8f
             - travelDistance * 0.35f
             - arenaPenalty;
    }

    PlannedSlot BuildPlan(PlayerbotAI* ai, Creature* gruul)
    {
        Player* bot = ai->GetBot();
        const uint32 selfGuid = bot->GetObjectGuid().GetCounter();
        const Vec2 current = Position2(bot);

        Vec2 best = current;
        float bestMinimum = MinimumDistanceToRaid(
            ai, current, selfGuid, true);
        float bestScore = CandidateScore(
            ai, gruul, current, current, selfGuid, bestMinimum);

        // The GUID phase removes directional symmetry. Reservations make
        // already-planned slots visible to later bots in the same map.
        const float phase =
            (float(selfGuid % 97u) / 97.0f) * TWO_PI;

        const float radii[] = { 8.0f, 14.0f, 20.0f };
        constexpr uint32 directionCount = 24;

        for (float radius : radii)
        {
            for (uint32 index = 0; index < directionCount; ++index)
            {
                const float angle =
                    phase + TWO_PI * float(index) / float(directionCount);

                Vec2 candidate(
                    current.x + std::cos(angle) * radius,
                    current.y + std::sin(angle) * radius);

                float candidateMinimum = 0.0f;
                const float score = CandidateScore(
                    ai,
                    gruul,
                    current,
                    candidate,
                    selfGuid,
                    candidateMinimum);

                if (score > bestScore)
                {
                    bestScore = score;
                    best = candidate;
                    bestMinimum = candidateMinimum;
                }
            }
        }

        return PlannedSlot(
            best.x,
            best.y,
            bot->GetPositionZ(),
            bestMinimum);
    }
}

float GruulShatterPlanner::CurrentMinimumSeparation(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return 999.0f;

    return MinimumDistanceToRaid(
        ai,
        Position2(ai->GetBot()),
        ai->GetBot()->GetObjectGuid().GetCounter(),
        false);
}

void GruulShatterPlanner::Reset(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return;

    Map* map = ai->GetBot()->GetMap();
    auto mapItr = s_shatterSlots.find(map);
    if (mapItr == s_shatterSlots.end())
        return;

    mapItr->second.erase(ai->GetBot()->GetObjectGuid().GetCounter());
    if (mapItr->second.empty())
        s_shatterSlots.erase(mapItr);
}

EncounterOverrideResult GruulShatterPlanner::Update(
    PlayerbotAI* ai,
    Creature* gruul)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return EncounterOverrideResult::NotHandled;

    Player* bot = ai->GetBot();

    if (!IsShatterWindow(ai, gruul))
    {
        Reset(ai);
        return EncounterOverrideResult::NotHandled;
    }

    PruneSlots(ai);

    Map* map = bot->GetMap();
    const uint32 selfGuid = bot->GetObjectGuid().GetCounter();
    SlotMap& slots = s_shatterSlots[map];

    auto slotItr = slots.find(selfGuid);
    if (slotItr == slots.end())
    {
        const float currentMinimum = CurrentMinimumSeparation(ai);
        const PlannedSlot plan = BuildPlan(ai, gruul);
        slotItr = slots.insert(std::make_pair(selfGuid, plan)).first;

        EncounterTrace::Event(
            ai,
            "GRUUL",
            "SHATTER_PLAN",
            "currentMin=%.2f plannedMin=%.2f acceptable=%u target=(%.2f,%.2f,%.2f)",
            currentMinimum,
            plan.predictedMinimum,
            plan.predictedMinimum >= ACCEPTABLE_SEPARATION ? 1u : 0u,
            plan.x,
            plan.y,
            plan.z);
    }

    PlannedSlot const& slot = slotItr->second;

    // Once Stoned has landed the core will reject movement. Keep the encounter
    // overlay authoritative so Normal Rotation cannot collapse the formation.
    if (bot->HasAura(EncounterConstants::SPELL_GRUUL_STONED))
        return EncounterOverrideResult::BlockNormal;

    if (bot->IsWithinDist3d(
            slot.x, slot.y, slot.z, MOVE_TOLERANCE))
    {
        return EncounterOverrideResult::BlockNormal;
    }

    bot->GetMotionMaster()->MovePoint(
        SHATTER_MOVE_ID_BASE + (selfGuid & 0xffu),
        slot.x,
        slot.y,
        slot.z,
        FORCED_MOVEMENT_RUN,
        true);

    return EncounterOverrideResult::BlockNormal;
}
