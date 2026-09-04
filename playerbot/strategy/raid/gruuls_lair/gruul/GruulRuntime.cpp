#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.h"

#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/strategy/raid/common/EncounterTypes.h"

#include "AI/ScriptDevAI/include/sc_grid_searchers.h"
#include "Maps/InstanceData.h"
#include "MotionGenerators/PathFinder.h"
#include "Util/Timer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

using namespace ai;

namespace
{
    constexpr float SEARCH_RANGE = 220.0f;
    constexpr float EPSILON = 0.001f;
    constexpr float MOVE_STEP = 5.0f;
    constexpr float PATH_LIMIT = 220.0f;
    constexpr uint32 MOVE_REISSUE_MS = 900;

    constexpr float MAIN_TANK_TOLERANCE = 2.5f;
    constexpr float SOAKER_TOLERANCE = 2.5f;
    constexpr float RANGED_TOLERANCE = 3.0f;

    constexpr float MAIN_TANK_OFFSET = 7.0f;
    constexpr float SOAKER_RANGE = 3.5f;
    constexpr uint32 RANGED_SLOTS_PER_RING = 9;
    constexpr float RANGED_INNER_RADIUS = 27.0f;
    constexpr float RANGED_OUTER_RADIUS = 35.0f;
    constexpr float RANGED_ARC_HALF_ANGLE = 1.3962634f; // 80 degrees

    constexpr uint32 MOVE_ID_BASE = 0x47525500; // "GRU\0"

    const uint32 PREFERRED_MAIN_TANKS[] = { 15, 24, 88 };
    const uint32 PREFERRED_SOAKER_WARRIORS[] = { 145, 100, 124 };
    const uint32 PREFERRED_SOAKER_PALADINS[] = { 31, 97, 98 };

    struct Vec2
    {
        float x;
        float y;

        Vec2() : x(0.0f), y(0.0f) {}
        Vec2(float px, float py) : x(px), y(py) {}

        Vec2 operator+(Vec2 const& rhs) const
        {
            return Vec2(x + rhs.x, y + rhs.y);
        }

        Vec2 operator-(Vec2 const& rhs) const
        {
            return Vec2(x - rhs.x, y - rhs.y);
        }

        Vec2 operator*(float scalar) const
        {
            return Vec2(x * scalar, y * scalar);
        }
    };

    struct Slot
    {
        float x;
        float y;
        float z;
        bool valid;

        Slot() : x(0.0f), y(0.0f), z(0.0f), valid(false) {}
        Slot(float px, float py, float pz)
            : x(px), y(py), z(pz), valid(true) {}
    };

    struct GruulState
    {
        GruulState()
            : initialized(false), map(nullptr), gruulGuid(),
              center(), entranceDir(), sideDir() {}

        bool initialized;
        Map* map;
        ObjectGuid gruulGuid;
        Slot center;
        Vec2 entranceDir;
        Vec2 sideDir;
        std::map<uint32, Slot> rangedSlots;
        std::map<uint32, uint32> lastMoveMs;
    };

    std::map<Map*, GruulState> s_states;

    float Length(Vec2 const& value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y);
    }

    Vec2 Normalize(Vec2 value)
    {
        const float length = Length(value);
        if (length < EPSILON)
            return Vec2(1.0f, 0.0f);

        return value * (1.0f / length);
    }

    Vec2 Perpendicular(Vec2 const& value)
    {
        return Vec2(-value.y, value.x);
    }

    Vec2 Position2(WorldObject const* object)
    {
        return object
            ? Vec2(object->GetPositionX(), object->GetPositionY())
            : Vec2();
    }

    float Distance2d(Slot const& lhs, WorldObject const* rhs)
    {
        if (!rhs || !lhs.valid)
            return 0.0f;

        const float dx = lhs.x - rhs->GetPositionX();
        const float dy = lhs.y - rhs->GetPositionY();
        return std::sqrt(dx * dx + dy * dy);
    }

    bool IsUsablePlayer(PlayerbotAI* ai, Player* player)
    {
        return ai && player && player->IsAlive() &&
               player->GetMap() == ai->GetBot()->GetMap();
    }

    Player* Preferred(PlayerbotAI* ai, const uint32* guids, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            EncounterActor actor = EncounterActorResolver::Find(ai, guids[i]);
            if (actor.IsValid() && actor.player->IsAlive())
                return actor.player;
        }

        return nullptr;
    }

    Player* HighestHealthTank(
        PlayerbotAI* ai,
        Player* excluded,
        uint8 preferredClass)
    {
        Player* best = nullptr;
        uint32 bestHealth = 0;

        for (Player* player : ai->GetPlayersInGroup())
        {
            if (!IsUsablePlayer(ai, player) || player == excluded ||
                !PlayerbotAI::IsTank(player, false))
            {
                continue;
            }

            if (preferredClass && player->getClass() != preferredClass)
                continue;

            if (!best || player->GetMaxHealth() > bestHealth)
            {
                best = player;
                bestHealth = player->GetMaxHealth();
            }
        }

        return best;
    }

    GruulState* State(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;

        auto itr = s_states.find(ai->GetBot()->GetMap());
        if (itr == s_states.end() || !itr->second.initialized)
            return nullptr;

        return &itr->second;
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

    bool CandidateValid(
        Player* actor,
        Creature* gruul,
        Slot& candidate,
        float minimumGruulDistance,
        float maximumGruulDistance)
    {
        if (!actor || !gruul || !candidate.valid)
            return false;

        actor->UpdateAllowedPositionZ(
            candidate.x, candidate.y, candidate.z);

        if (!actor->IsWithinLOS(
                candidate.x, candidate.y, candidate.z))
        {
            return false;
        }

        if (!gruul->IsWithinLOS(
                candidate.x, candidate.y, candidate.z))
        {
            return false;
        }

        const float distance = Distance2d(candidate, gruul);
        if (minimumGruulDistance > 0.0f &&
            distance < minimumGruulDistance)
        {
            return false;
        }

        if (maximumGruulDistance > 0.0f &&
            distance > maximumGruulDistance)
        {
            return false;
        }

        return PathAccepts(actor, candidate);
    }

    bool AtSlot(Player const* player, Slot const& slot, float tolerance)
    {
        if (!player || !slot.valid)
            return false;

        const float dx = player->GetPositionX() - slot.x;
        const float dy = player->GetPositionY() - slot.y;
        const float dz = std::fabs(player->GetPositionZ() - slot.z);
        return dx * dx + dy * dy <= tolerance * tolerance && dz <= 4.0f;
    }

    bool MoveToward(
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

        GruulState* state = State(ai);
        if (!state)
            return false;

        const uint32 guid = bot->GetObjectGuid().GetCounter();
        const uint32 now = WorldTimer::getMSTime();
        const uint32 last = state->lastMoveMs[guid];
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
        Slot step(x, y, z);

        if (!bot->IsWithinLOS(step.x, step.y, step.z) ||
            !PathAccepts(bot, step))
        {
            return false;
        }

        bot->GetMotionMaster()->MovePoint(
            moveId, step.x, step.y, step.z,
            FORCED_MOVEMENT_RUN, true);

        state->lastMoveMs[guid] = now;
        return true;
    }

    bool EnsureState(PlayerbotAI* ai, Creature* gruul)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap() ||
            !gruul || !gruul->IsAlive())
        {
            return false;
        }

        Map* map = ai->GetBot()->GetMap();
        GruulState& state = s_states[map];
        if (state.initialized && state.gruulGuid == gruul->GetObjectGuid())
            return true;

        float centerX = gruul->GetPositionX();
        float centerY = gruul->GetPositionY();
        float centerZ = gruul->GetPositionZ();
        gruul->GetRespawnCoord(centerX, centerY, centerZ);

        Vec2 raidCenter;
        uint32 raidCount = 0;
        for (Player* player : ai->GetPlayersInGroup())
        {
            if (!IsUsablePlayer(ai, player))
                continue;

            raidCenter = raidCenter + Position2(player);
            ++raidCount;
        }

        if (raidCount)
            raidCenter = raidCenter * (1.0f / static_cast<float>(raidCount));
        else
            raidCenter = Position2(ai->GetBot());

        const Vec2 center(centerX, centerY);
        Vec2 entrance = Normalize(raidCenter - center);
        if (Length(raidCenter - center) < 2.0f)
            entrance = Normalize(Position2(ai->GetBot()) - center);

        state = GruulState();
        state.initialized = true;
        state.map = map;
        state.gruulGuid = gruul->GetObjectGuid();
        state.center = Slot(centerX, centerY, centerZ);
        state.entranceDir = entrance;
        state.sideDir = Perpendicular(entrance);

        sLog.outDetail(
            "[EncounterAI][Gruul] frame captured center=(%.2f,%.2f,%.2f) "
            "source=GRUUL_RESPAWN_PLUS_RAID_CENTROID",
            centerX, centerY, centerZ);

        return true;
    }

    uint32 RangedOrdinal(PlayerbotAI* ai, Player* actor)
    {
        std::vector<uint32> guids;

        for (Player* player : ai->GetPlayersInGroup())
        {
            if (!IsUsablePlayer(ai, player))
                continue;

            if (!PlayerbotAI::IsHeal(player, false) &&
                !ai->IsRanged(player, false))
            {
                continue;
            }

            guids.push_back(player->GetObjectGuid().GetCounter());
        }

        std::sort(guids.begin(), guids.end());
        guids.erase(std::unique(guids.begin(), guids.end()), guids.end());

        const uint32 self = actor->GetObjectGuid().GetCounter();
        auto itr = std::find(guids.begin(), guids.end(), self);
        return itr == guids.end()
            ? 0
            : static_cast<uint32>(std::distance(guids.begin(), itr));
    }

    Slot ResolveMainTankSlot(
        PlayerbotAI* ai,
        Creature* gruul,
        GruulState const& state)
    {
        Slot ideal(
            state.center.x - state.entranceDir.x * MAIN_TANK_OFFSET,
            state.center.y - state.entranceDir.y * MAIN_TANK_OFFSET,
            state.center.z);

        static const float sideOffsets[] =
            { 0.0f, 2.0f, -2.0f, 4.0f, -4.0f };
        static const float forwardOffsets[] =
            { 0.0f, 2.0f, -2.0f, 4.0f, -4.0f };

        for (float forward : forwardOffsets)
        {
            for (float side : sideOffsets)
            {
                Slot candidate = ideal;
                candidate.x +=
                    state.entranceDir.x * forward +
                    state.sideDir.x * side;
                candidate.y +=
                    state.entranceDir.y * forward +
                    state.sideDir.y * side;

                if (CandidateValid(
                        ai->GetBot(), gruul, candidate, 0.0f, 0.0f))
                {
                    return candidate;
                }
            }
        }

        return Slot();
    }

    Slot ResolveSoakerSlot(
        PlayerbotAI* ai,
        Creature* gruul,
        GruulState const& state)
    {
        const Vec2 boss = Position2(gruul);
        const Vec2 behind = state.entranceDir;
        const Vec2 candidatePosition =
            boss + behind * SOAKER_RANGE +
            state.sideDir * 2.0f;

        Slot candidate(
            candidatePosition.x,
            candidatePosition.y,
            gruul->GetPositionZ());

        if (CandidateValid(
                ai->GetBot(), gruul, candidate, 2.0f, 8.0f))
        {
            return candidate;
        }

        candidate = Slot(
            boss.x + behind.x * SOAKER_RANGE -
                state.sideDir.x * 2.0f,
            boss.y + behind.y * SOAKER_RANGE -
                state.sideDir.y * 2.0f,
            gruul->GetPositionZ());

        return CandidateValid(
                ai->GetBot(), gruul, candidate, 2.0f, 8.0f)
            ? candidate
            : Slot();
    }

    Slot ResolveRangedSlot(
        PlayerbotAI* ai,
        Creature* gruul,
        GruulState const& state)
    {
        Player* bot = ai->GetBot();
        const uint32 ordinal = RangedOrdinal(ai, bot);
        const uint32 lane = ordinal % RANGED_SLOTS_PER_RING;
        const uint32 ring = (ordinal / RANGED_SLOTS_PER_RING) % 2;

        const float radius = ring
            ? RANGED_OUTER_RADIUS
            : RANGED_INNER_RADIUS;

        const float laneStep =
            (2.0f * RANGED_ARC_HALF_ANGLE) /
            static_cast<float>(RANGED_SLOTS_PER_RING - 1);
        float angle =
            std::atan2(state.entranceDir.y, state.entranceDir.x) -
            RANGED_ARC_HALF_ANGLE +
            static_cast<float>(lane) * laneStep;

        // Offset the outer ring by half a lane to prevent radial stacking.
        if (ring)
            angle += laneStep * 0.5f;

        static const float angleOffsets[] =
        {
            0.0f, 0.10f, -0.10f, 0.20f, -0.20f, 0.30f, -0.30f
        };
        static const float radiusOffsets[] =
        {
            0.0f, 2.0f, -2.0f, 4.0f
        };

        for (float radiusOffset : radiusOffsets)
        {
            for (float angleOffset : angleOffsets)
            {
                const float useRadius = radius + radiusOffset;
                const float useAngle = angle + angleOffset;
                Slot candidate(
                    gruul->GetPositionX() +
                        std::cos(useAngle) * useRadius,
                    gruul->GetPositionY() +
                        std::sin(useAngle) * useRadius,
                    gruul->GetPositionZ());

                if (CandidateValid(
                        bot, gruul, candidate, 24.0f, 40.0f))
                {
                    return candidate;
                }
            }
        }

        return Slot();
    }
}

Creature* GruulRuntime::FindGruul(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    return GetClosestCreatureWithEntry(
        ai->GetBot(),
        EncounterConstants::NPC_GRUUL,
        SEARCH_RANGE,
        true);
}

bool GruulRuntime::IsEncounterInProgress(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() ||
        ai->GetBot()->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
    {
        return false;
    }

    InstanceData* instance =
        ai->GetBot()->GetMap()
            ? ai->GetBot()->GetMap()->GetInstanceData()
            : nullptr;

    return instance &&
           instance->GetData(EncounterConstants::TYPE_GRUUL_EVENT) ==
               EncounterConstants::ENCOUNTER_IN_PROGRESS;
}

bool GruulRuntime::IsShatterWindow(
    PlayerbotAI* ai,
    Creature* gruul)
{
    if (!ai || !ai->GetBot())
        return false;

    if (!gruul)
        gruul = FindGruul(ai);

    Player* bot = ai->GetBot();
    return
        (gruul &&
         (gruul->HasAura(EncounterConstants::SPELL_GRUUL_GROUND_SLAM) ||
          gruul->HasAura(
              EncounterConstants::SPELL_GRUUL_GROUND_SLAM_TRIGGER) ||
          gruul->HasAura(
              EncounterConstants::SPELL_GRUUL_GROUND_SLAM_DUMMY))) ||
        bot->HasAura(EncounterConstants::SPELL_GRUUL_GROUND_SLAM) ||
        bot->HasAura(
            EncounterConstants::SPELL_GRUUL_GROUND_SLAM_TRIGGER) ||
        bot->HasAura(EncounterConstants::SPELL_GRUUL_STONED) ||
        bot->HasAura(EncounterConstants::SPELL_GRUUL_SHATTER) ||
        bot->HasAura(EncounterConstants::SPELL_GRUUL_SHATTER_EFFECT);
}

Player* GruulRuntime::ResolveMainTank(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    if (Player* preferred = Preferred(
            ai,
            PREFERRED_MAIN_TANKS,
            sizeof(PREFERRED_MAIN_TANKS) / sizeof(uint32)))
    {
        return preferred;
    }

    if (Player* druid = HighestHealthTank(ai, nullptr, CLASS_DRUID))
        return druid;

    return HighestHealthTank(ai, nullptr, 0);
}

Player* GruulRuntime::ResolveHurtfulSoaker(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    Player* mainTank = ResolveMainTank(ai);

    if (Player* preferred = Preferred(
            ai,
            PREFERRED_SOAKER_WARRIORS,
            sizeof(PREFERRED_SOAKER_WARRIORS) / sizeof(uint32)))
    {
        if (preferred != mainTank)
            return preferred;
    }

    if (Player* preferred = Preferred(
            ai,
            PREFERRED_SOAKER_PALADINS,
            sizeof(PREFERRED_SOAKER_PALADINS) / sizeof(uint32)))
    {
        if (preferred != mainTank)
            return preferred;
    }

    if (Player* warrior =
            HighestHealthTank(ai, mainTank, CLASS_WARRIOR))
    {
        return warrior;
    }

    if (Player* paladin =
            HighestHealthTank(ai, mainTank, CLASS_PALADIN))
    {
        return paladin;
    }

    return HighestHealthTank(ai, mainTank, 0);
}

bool GruulRuntime::IsMainTank(PlayerbotAI* ai)
{
    Player* tank = ResolveMainTank(ai);
    return tank && ai && ai->GetBot() == tank;
}

bool GruulRuntime::IsHurtfulSoaker(PlayerbotAI* ai)
{
    Player* soaker = ResolveHurtfulSoaker(ai);
    return soaker && ai && ai->GetBot() == soaker;
}

bool GruulRuntime::IsRangedOrHealer(PlayerbotAI* ai)
{
    return ai && ai->GetBot() &&
        (PlayerbotAI::IsHeal(ai->GetBot(), false) ||
         ai->IsRanged(ai->GetBot(), false));
}

void GruulRuntime::SetEncounterTarget(
    PlayerbotAI* ai,
    Creature* gruul)
{
    if (!ai || !gruul || !gruul->IsAlive())
        return;

    AiObjectContext* context = ai->GetAiObjectContext();
    if (!context)
        return;

    context->GetValue<Unit*>("current target")->Set(gruul);
    context->GetValue<ObjectGuid>("attack target")
        ->Set(gruul->GetObjectGuid());
}

bool GruulRuntime::MaintainMainTankPosition(
    PlayerbotAI* ai,
    Creature* gruul)
{
    if (!IsMainTank(ai) || !EnsureState(ai, gruul))
        return false;

    // Do not run ahead of the pull. Once Gruul is actually attacking the
    // assigned tank, drag him to the dynamically derived room-center slot.
    if (gruul->GetVictim() != ai->GetBot())
        return false;

    GruulState* state = State(ai);
    Slot slot = ResolveMainTankSlot(ai, gruul, *state);
    return MoveToward(
        ai, slot, MAIN_TANK_TOLERANCE, MOVE_ID_BASE + 1);
}

bool GruulRuntime::MaintainHurtfulSoakerPosition(
    PlayerbotAI* ai,
    Creature* gruul)
{
    if (!IsHurtfulSoaker(ai) || !EnsureState(ai, gruul))
        return false;

    GruulState* state = State(ai);
    Slot slot = ResolveSoakerSlot(ai, gruul, *state);
    return MoveToward(
        ai, slot, SOAKER_TOLERANCE, MOVE_ID_BASE + 2);
}

bool GruulRuntime::MaintainRangedSpread(
    PlayerbotAI* ai,
    Creature* gruul)
{
    if (!IsRangedOrHealer(ai) ||
        IsMainTank(ai) ||
        IsHurtfulSoaker(ai) ||
        !EnsureState(ai, gruul))
    {
        return false;
    }

    GruulState* state = State(ai);
    const uint32 guid = ai->GetBot()->GetObjectGuid().GetCounter();
    Slot slot = ResolveRangedSlot(ai, gruul, *state);
    if (!slot.valid)
        return false;

    state->rangedSlots[guid] = slot;
    return MoveToward(
        ai,
        slot,
        RANGED_TOLERANCE,
        MOVE_ID_BASE + 20 + (RangedOrdinal(ai, ai->GetBot()) % 40));
}

bool GruulRuntime::ShouldDelayBloodlust(PlayerbotAI* ai)
{
    Creature* gruul = FindGruul(ai);
    return IsEncounterInProgress(ai) &&
           gruul && gruul->IsAlive() &&
           gruul->GetHealthPercent() >= 95.0f;
}

bool GruulRuntime::ShouldLockMainTankMovement(PlayerbotAI* ai)
{
    Creature* gruul = FindGruul(ai);
    return IsEncounterInProgress(ai) &&
           IsMainTank(ai) &&
           gruul &&
           gruul->GetVictim() == ai->GetBot();
}

void GruulRuntime::Reset(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return;

    s_states.erase(ai->GetBot()->GetMap());
}
