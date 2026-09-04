#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFormationManager.h"
#include "playerbot/PlayerbotAI.h"

#include "MotionGenerators/PathFinder.h"
#include "Util/Timer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

using namespace ai;

namespace
{
    constexpr float FORMATION_EPS = 0.001f;
    constexpr float PREP_MOVE_STEP = 5.0f;
    constexpr float POSITION_PATH_LIMIT = 220.0f;
    constexpr uint32 MOVE_REISSUE_MS = 900;

    constexpr float TANK_MOVE_TOLERANCE = 2.0f;
    constexpr float RANGED_MOVE_TOLERANCE = 2.5f;
    constexpr float HEALER_MOVE_TOLERANCE = 3.0f;
    constexpr float MELEE_MOVE_TOLERANCE = 1.75f;

    constexpr float MAULGAR_PULL_DISTANCE = 22.0f;
    constexpr float BLINDEYE_PULL_DISTANCE = 17.0f;
    constexpr float OLM_PULL_DISTANCE = 17.0f;
    constexpr float COUNCIL_LATERAL_OFFSET = 7.0f;

    constexpr float KROSH_MAGE_RANGE = 26.0f;
    constexpr float KIGGLER_TANK_RANGE = 28.5f;
    constexpr float OLM_WARLOCK_RANGE = 23.0f;

    constexpr float HEALER_BACKLINE_DISTANCE = 33.0f;
    constexpr float RANGED_TARGET_RANGE = 24.0f;
    constexpr float MELEE_REAR_RANGE = 3.0f;

    constexpr float WHIRLWIND_SAFE_RANGE = 18.0f;
    constexpr float WHIRLWIND_ESCAPE_RANGE = 21.0f;

    constexpr float PREP_COUNCIL_CLEARANCE = 18.0f;
    constexpr uint32 FORMATION_MOVE_ID_BASE = 0x4D415500; // "MAU\0"
    constexpr uint32 PREP_MOVE_ID_BASE = 0x4D415000;      // "MAP\0"

    enum CouncilIndex : uint8
    {
        COUNCIL_MAULGAR = 0,
        COUNCIL_KROSH,
        COUNCIL_OLM,
        COUNCIL_KIGGLER,
        COUNCIL_BLINDEYE,
        COUNCIL_COUNT
    };

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
        float o;
        bool valid;

        Slot()
            : x(0.0f), y(0.0f), z(0.0f), o(0.0f), valid(false) {}

        Slot(float px, float py, float pz, float po = 0.0f)
            : x(px), y(py), z(pz), o(po), valid(true) {}
    };

    struct PreparationSlotKey
    {
        uint32 guid;
        uint8 role;

        PreparationSlotKey(uint32 actorGuid, MaulgarPreparationRole actorRole)
            : guid(actorGuid), role(static_cast<uint8>(actorRole)) {}

        bool operator<(PreparationSlotKey const& rhs) const
        {
            if (guid != rhs.guid)
                return guid < rhs.guid;
            return role < rhs.role;
        }
    };

    struct MaulgarFormationState
    {
        MaulgarFormationState()
            : initialized(false), map(nullptr) {}

        bool initialized;
        Map* map;
        Vec2 councilCenter;
        Vec2 entranceDir;
        Vec2 sideDir;

        Slot council[COUNCIL_COUNT];
        ObjectGuid councilGuids[COUNCIL_COUNT];

        Slot maulgarAnchor;
        Slot blindeyeAnchor;
        Slot olmAnchor;
        Slot felStandbyAnchor;

        std::map<PreparationSlotKey, Slot> preparationSlots;
        std::map<uint32, uint32> lastMoveMs;
    };

    std::map<Map*, MaulgarFormationState> s_maulgarFormation;

    float Length(Vec2 const& value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y);
    }

    float Distance2d(Vec2 const& lhs, Vec2 const& rhs)
    {
        return Length(lhs - rhs);
    }

    Vec2 Normalize(Vec2 value)
    {
        const float length = Length(value);
        if (length < FORMATION_EPS)
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

    Slot ObjectSlot(WorldObject const* object)
    {
        return object
            ? Slot(
                  object->GetPositionX(),
                  object->GetPositionY(),
                  object->GetPositionZ(),
                  object->GetOrientation())
            : Slot();
    }

    Slot MakeSlot(Vec2 const& position, float z, float orientation = 0.0f)
    {
        return Slot(position.x, position.y, z, orientation);
    }

    MaulgarFormationState* State(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;

        auto itr = s_maulgarFormation.find(ai->GetBot()->GetMap());
        if (itr == s_maulgarFormation.end() || !itr->second.initialized)
            return nullptr;

        return &itr->second;
    }

    bool IsProtectedHuman(Player const* player)
    {
        if (!player)
            return false;

        PlayerbotAI* actorAI = player->GetPlayerbotAI();
        return !actorAI || actorAI->IsRealPlayer();
    }

    uint32 GroupOrdinal(PlayerbotAI* ai, Player const* actor = nullptr)
    {
        if (!ai || !ai->GetBot())
            return 0;

        if (!actor)
            actor = ai->GetBot();

        std::vector<uint32> guids;
        for (Player* player : ai->GetPlayersInGroup())
        {
            if (!player || !player->IsAlive() ||
                player->GetMap() != actor->GetMap() ||
                IsProtectedHuman(player))
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

    int TargetIndex(MaulgarPreparationRole role)
    {
        switch (role)
        {
            case MaulgarPreparationRole::MaulgarTank:
            case MaulgarPreparationRole::HunterMaulgar:
                return COUNCIL_MAULGAR;
            case MaulgarPreparationRole::KroshMage:
                return COUNCIL_KROSH;
            case MaulgarPreparationRole::OlmWarlock:
                return COUNCIL_OLM;
            case MaulgarPreparationRole::KigglerTank:
            case MaulgarPreparationRole::HunterKiggler:
                return COUNCIL_KIGGLER;
            case MaulgarPreparationRole::BlindeyeTank:
            case MaulgarPreparationRole::HunterBlindeye:
                return COUNCIL_BLINDEYE;
            default:
                return -1;
        }
    }

    Creature* TargetForRole(
        MaulgarFormationState const& state,
        MaulgarPreparationRole role)
    {
        const int index = TargetIndex(role);
        if (index < 0 || index >= COUNCIL_COUNT || !state.map)
            return nullptr;

        return state.map->GetCreature(state.councilGuids[index]);
    }

    float PreparationTolerance(MaulgarPreparationRole role)
    {
        switch (role)
        {
            case MaulgarPreparationRole::HealerBackline:
            case MaulgarPreparationRole::RangedBackline:
            case MaulgarPreparationRole::MeleeBackline:
                return 3.5f;
            default:
                return 2.5f;
        }
    }

    float PreparationMaximumTargetDistance(MaulgarPreparationRole role)
    {
        switch (role)
        {
            case MaulgarPreparationRole::KroshMage:
            case MaulgarPreparationRole::OlmWarlock:
                return 32.0f;
            case MaulgarPreparationRole::KigglerTank:
                return 35.0f;
            case MaulgarPreparationRole::HunterMaulgar:
            case MaulgarPreparationRole::HunterBlindeye:
            case MaulgarPreparationRole::HunterKiggler:
                return 39.0f;
            case MaulgarPreparationRole::MaulgarTank:
            case MaulgarPreparationRole::BlindeyeTank:
                return 45.0f;
            default:
                return 0.0f;
        }
    }

    Slot TargetRelativeSlot(
        MaulgarFormationState const& state,
        int targetIndex,
        float range,
        float lateral)
    {
        if (targetIndex < 0 || targetIndex >= COUNCIL_COUNT ||
            !state.council[targetIndex].valid)
        {
            return Slot();
        }

        const Slot& target = state.council[targetIndex];
        const Vec2 targetPosition(target.x, target.y);
        const Vec2 entranceReference =
            state.councilCenter + state.entranceDir * 42.0f;
        const Vec2 towardEntrance =
            Normalize(entranceReference - targetPosition);
        const Vec2 side = Perpendicular(towardEntrance);
        const Vec2 position =
            targetPosition + towardEntrance * range + side * lateral;
        const float facing = std::atan2(
            targetPosition.y - position.y,
            targetPosition.x - position.x);

        return Slot(position.x, position.y, target.z, facing);
    }

    Slot RelativeTargetSlot(
        MaulgarFormationState const& state,
        WorldObject const* target,
        float range,
        float lateral)
    {
        if (!target)
            return Slot();

        const Vec2 targetPosition = Position2(target);
        const Vec2 entranceReference =
            state.councilCenter + state.entranceDir * 42.0f;
        const Vec2 towardEntrance =
            Normalize(entranceReference - targetPosition);
        const Vec2 side = Perpendicular(towardEntrance);
        const Vec2 position =
            targetPosition + towardEntrance * range + side * lateral;

        return Slot(
            position.x,
            position.y,
            target->GetPositionZ(),
            std::atan2(
                targetPosition.y - position.y,
                targetPosition.x - position.x));
    }

    Slot IdealPreparationSlot(
        PlayerbotAI* ai,
        Player* actor,
        MaulgarFormationState const& state,
        MaulgarPreparationRole role)
    {
        switch (role)
        {
            case MaulgarPreparationRole::MaulgarTank:
                return TargetRelativeSlot(state, COUNCIL_MAULGAR, 27.0f, -8.0f);
            case MaulgarPreparationRole::BlindeyeTank:
                return TargetRelativeSlot(state, COUNCIL_BLINDEYE, 27.0f, 8.0f);
            case MaulgarPreparationRole::KigglerTank:
                return TargetRelativeSlot(state, COUNCIL_KIGGLER, 30.0f, -9.0f);
            case MaulgarPreparationRole::KroshMage:
                return TargetRelativeSlot(state, COUNCIL_KROSH, 27.0f, 9.0f);
            case MaulgarPreparationRole::OlmWarlock:
                return TargetRelativeSlot(state, COUNCIL_OLM, 27.0f, 3.0f);
            case MaulgarPreparationRole::FelhunterStandby:
            {
                const Vec2 position =
                    state.councilCenter + state.entranceDir * 31.0f;
                return MakeSlot(
                    position,
                    state.council[COUNCIL_OLM].z,
                    std::atan2(
                        state.councilCenter.y - position.y,
                        state.councilCenter.x - position.x));
            }
            case MaulgarPreparationRole::HunterMaulgar:
                return TargetRelativeSlot(state, COUNCIL_MAULGAR, 33.0f, -13.0f);
            case MaulgarPreparationRole::HunterBlindeye:
                return TargetRelativeSlot(state, COUNCIL_BLINDEYE, 33.0f, 13.0f);
            case MaulgarPreparationRole::HunterKiggler:
                return TargetRelativeSlot(state, COUNCIL_KIGGLER, 33.0f, -2.0f);
            case MaulgarPreparationRole::HealerBackline:
            case MaulgarPreparationRole::RangedBackline:
            case MaulgarPreparationRole::MeleeBackline:
            {
                const uint32 ordinal = GroupOrdinal(ai, actor);
                float distance = 33.0f;
                float spacing = 3.0f;
                uint32 laneCount = 9;

                if (role == MaulgarPreparationRole::HealerBackline)
                {
                    distance = 37.0f;
                    spacing = 3.3f;
                    laneCount = 7;
                }
                else if (role == MaulgarPreparationRole::MeleeBackline)
                {
                    distance = 29.0f;
                    spacing = 2.6f;
                    laneCount = 9;
                }

                const int32 lane =
                    static_cast<int32>(ordinal % laneCount) -
                    static_cast<int32>(laneCount / 2);
                const Vec2 position =
                    state.councilCenter +
                    state.entranceDir * distance +
                    state.sideDir * (static_cast<float>(lane) * spacing);

                return MakeSlot(
                    position,
                    state.council[COUNCIL_MAULGAR].z,
                    std::atan2(
                        state.councilCenter.y - position.y,
                        state.councilCenter.x - position.x));
            }
            default:
                return Slot();
        }
    }

    bool CouncilClear(
        MaulgarFormationState const& state,
        Slot const& slot)
    {
        const Vec2 candidate(slot.x, slot.y);
        for (uint8 index = 0; index < COUNCIL_COUNT; ++index)
        {
            if (!state.council[index].valid)
                return false;

            const Vec2 councilPosition(
                state.council[index].x,
                state.council[index].y);

            if (Distance2d(candidate, councilPosition) <
                PREP_COUNCIL_CLEARANCE)
            {
                return false;
            }
        }

        return true;
    }

    bool PathAccepts(Player* actor, Slot const& slot)
    {
        if (!actor || !slot.valid)
            return false;

        PathFinder path(actor);
        path.setPathLengthLimit(POSITION_PATH_LIMIT);
        path.calculate(slot.x, slot.y, slot.z, false, false);

        const uint32 pathType = static_cast<uint32>(path.getPathType());
        if (pathType & static_cast<uint32>(PATHFIND_NOPATH))
            return false;
        if (pathType & static_cast<uint32>(PATHFIND_SHORTCUT))
            return false;
        if (pathType & static_cast<uint32>(PATHFIND_SHORT))
            return false;
        if (pathType & static_cast<uint32>(PATHFIND_INCOMPLETE))
            return false;

        return pathType &
            (static_cast<uint32>(PATHFIND_NORMAL) |
             static_cast<uint32>(PATHFIND_NOT_USING_PATH));
    }

    bool CandidateValid(
        Player* actor,
        MaulgarFormationState const& state,
        MaulgarPreparationRole role,
        Slot& candidate)
    {
        if (!actor || !candidate.valid)
            return false;

        actor->UpdateAllowedPositionZ(
            candidate.x,
            candidate.y,
            candidate.z);

        if (!actor->IsWithinLOS(
                candidate.x,
                candidate.y,
                candidate.z))
        {
            return false;
        }

        if (!CouncilClear(state, candidate))
            return false;

        Creature* target = TargetForRole(state, role);
        const float maximumTargetDistance =
            PreparationMaximumTargetDistance(role);

        if (target)
        {
            const Vec2 targetPosition = Position2(target);
            const float distance = Distance2d(
                Vec2(candidate.x, candidate.y),
                targetPosition);

            if (maximumTargetDistance > 0.0f &&
                distance > maximumTargetDistance)
            {
                return false;
            }

            if (!target->IsWithinLOS(
                    candidate.x,
                    candidate.y,
                    candidate.z))
            {
                return false;
            }

            candidate.o = std::atan2(
                target->GetPositionY() - candidate.y,
                target->GetPositionX() - candidate.x);
        }

        return PathAccepts(actor, candidate);
    }

    Slot ResolvePreparationSlot(
        PlayerbotAI* ai,
        Player* actor,
        MaulgarFormationState const& state,
        MaulgarPreparationRole role)
    {
        Slot ideal = IdealPreparationSlot(ai, actor, state, role);
        if (!ideal.valid)
            return Slot();

        static const float forwardOffsets[] =
        {
            0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f
        };
        static const float sideOffsets[] =
        {
            0.0f, 2.0f, -2.0f, 4.0f, -4.0f, 6.0f, -6.0f,
            8.0f, -8.0f
        };

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

                if (CandidateValid(actor, state, role, candidate))
                    return candidate;
            }
        }

        sLog.outError(
            "[EncounterAI][Maulgar][Formation] no reachable preparation "
            "slot role=%s actor=%s guid=%u",
            MaulgarFormationManager::PreparationRoleName(role),
            actor->GetName(),
            actor->GetObjectGuid().GetCounter());

        return Slot();
    }

    Slot* PreparationSlot(
        PlayerbotAI* ai,
        Player* actor,
        MaulgarPreparationRole role)
    {
        MaulgarFormationState* state = State(ai);
        if (!state || !actor)
            return nullptr;

        const PreparationSlotKey key(
            actor->GetObjectGuid().GetCounter(),
            role);
        auto itr = state->preparationSlots.find(key);
        if (itr != state->preparationSlots.end())
            return itr->second.valid ? &itr->second : nullptr;

        Slot slot = ResolvePreparationSlot(ai, actor, *state, role);
        if (!slot.valid)
            return nullptr;

        state->preparationSlots[key] = slot;

        sLog.outDetail(
            "[EncounterAI][Maulgar][Formation] preparation role=%s "
            "actor=%s guid=%u slot=(%.2f,%.2f,%.2f)",
            MaulgarFormationManager::PreparationRoleName(role),
            actor->GetName(),
            actor->GetObjectGuid().GetCounter(),
            slot.x,
            slot.y,
            slot.z);

        return &state->preparationSlots[key];
    }

    bool AtSlot(Player const* actor, Slot const& slot, float tolerance)
    {
        if (!actor || !slot.valid)
            return false;

        const float dx = actor->GetPositionX() - slot.x;
        const float dy = actor->GetPositionY() - slot.y;
        const float dz = std::fabs(actor->GetPositionZ() - slot.z);
        return (dx * dx + dy * dy) <= tolerance * tolerance && dz <= 4.0f;
    }

    bool MoveTowardSlot(
        PlayerbotAI* ai,
        Slot const& finalSlot,
        float tolerance,
        uint32 moveId,
        bool holdAtDestination)
    {
        if (!ai || !ai->GetBot() || !finalSlot.valid)
            return false;

        Player* bot = ai->GetBot();
        if (IsProtectedHuman(bot))
            return false;

        if (AtSlot(bot, finalSlot, tolerance))
        {
            if (holdAtDestination &&
                bot->GetMotionMaster()->GetCurrentMovementGeneratorType() !=
                    STAY_MOTION_TYPE)
            {
                bot->GetMotionMaster()->MoveStay(
                    finalSlot.x,
                    finalSlot.y,
                    finalSlot.z,
                    finalSlot.o,
                    true);
            }

            return false;
        }

        MaulgarFormationState* state = State(ai);
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

        float destinationX = finalSlot.x;
        float destinationY = finalSlot.y;
        float destinationZ = finalSlot.z;

        // Match the upstream Maulgar action pattern: advance toward an anchor
        // in at most 5-yard increments instead of replacing the whole combat
        // path on every AI update.
        if (distance > PREP_MOVE_STEP)
        {
            destinationX =
                bot->GetPositionX() + dx / distance * PREP_MOVE_STEP;
            destinationY =
                bot->GetPositionY() + dy / distance * PREP_MOVE_STEP;
            destinationZ = bot->GetPositionZ();
        }

        bot->UpdateAllowedPositionZ(
            destinationX,
            destinationY,
            destinationZ);

        Slot step(
            destinationX,
            destinationY,
            destinationZ,
            finalSlot.o);
        if (!bot->IsWithinLOS(step.x, step.y, step.z) ||
            !PathAccepts(bot, step))
        {
            sLog.outError(
                "[EncounterAI][Maulgar][Formation] rejected movement "
                "actor=%s guid=%u dest=(%.2f,%.2f,%.2f)",
                bot->GetName(),
                guid,
                step.x,
                step.y,
                step.z);
            return false;
        }

        bot->GetMotionMaster()->MovePoint(
            moveId,
            step.x,
            step.y,
            step.z,
            FORCED_MOVEMENT_RUN,
            true);

        state->lastMoveMs[guid] = now;
        return true;
    }

    bool HumanRangeReady(
        MaulgarFormationState const& state,
        Player* actor,
        MaulgarPreparationRole role)
    {
        Creature* target = TargetForRole(state, role);
        if (!actor || !target || !target->IsAlive())
            return false;

        const float distance = actor->GetDistance2d(
            target->GetPositionX(),
            target->GetPositionY());
        float minimum = PREP_COUNCIL_CLEARANCE;
        float maximum = PreparationMaximumTargetDistance(role);

        if (role == MaulgarPreparationRole::KroshMage)
        {
            minimum = 20.0f;
            maximum = 32.0f;
        }
        else if (role == MaulgarPreparationRole::OlmWarlock)
        {
            minimum = 18.0f;
            maximum = 32.0f;
        }

        return distance >= minimum &&
               distance <= maximum &&
               actor->IsWithinLOS(
                   target->GetPositionX(),
                   target->GetPositionY(),
                   target->GetPositionZ());
    }
}

void MaulgarFormationManager::Reset(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return;

    s_maulgarFormation.erase(ai->GetBot()->GetMap());
}

bool MaulgarFormationManager::EnsureMaulgarFrame(
    PlayerbotAI* ai,
    Creature* maulgar,
    Creature* krosh,
    Creature* olm,
    Creature* kiggler,
    Creature* blindeye)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap() ||
        !maulgar || !krosh || !olm || !kiggler || !blindeye)
    {
        return false;
    }

    Map* map = ai->GetBot()->GetMap();
    MaulgarFormationState& state = s_maulgarFormation[map];
    if (state.initialized)
        return true;

    WorldObject* council[COUNCIL_COUNT] =
    {
        maulgar,
        krosh,
        olm,
        kiggler,
        blindeye
    };

    Vec2 center;
    float zAverage = 0.0f;
    for (uint8 index = 0; index < COUNCIL_COUNT; ++index)
    {
        state.council[index] = ObjectSlot(council[index]);
        state.councilGuids[index] = council[index]->GetObjectGuid();
        center = center + Position2(council[index]);
        zAverage += council[index]->GetPositionZ();
    }

    center = center * (1.0f / static_cast<float>(COUNCIL_COUNT));
    zAverage /= static_cast<float>(COUNCIL_COUNT);

    Vec2 raidCenter;
    uint32 raidCount = 0;
    for (Player* player : ai->GetPlayersInGroup())
    {
        if (!player || !player->IsAlive() || player->GetMap() != map)
            continue;

        raidCenter = raidCenter + Position2(player);
        ++raidCount;
    }

    if (raidCount)
        raidCenter = raidCenter * (1.0f / static_cast<float>(raidCount));
    else
        raidCenter = Position2(ai->GetBot());

    Vec2 entranceDir = Normalize(raidCenter - center);
    if (Distance2d(raidCenter, center) < 2.0f)
        entranceDir = Normalize(Position2(maulgar) - center);

    state.map = map;
    state.councilCenter = center;
    state.entranceDir = entranceDir;
    state.sideDir = Perpendicular(entranceDir);

    state.maulgarAnchor = MakeSlot(
        center +
            entranceDir * MAULGAR_PULL_DISTANCE +
            state.sideDir * 10.0f,
        maulgar->GetPositionZ());

    state.blindeyeAnchor = MakeSlot(
        center -
            entranceDir * BLINDEYE_PULL_DISTANCE +
            state.sideDir * COUNCIL_LATERAL_OFFSET,
        blindeye->GetPositionZ());

    state.olmAnchor = MakeSlot(
        center -
            entranceDir * OLM_PULL_DISTANCE -
            state.sideDir * COUNCIL_LATERAL_OFFSET,
        olm->GetPositionZ());

    state.felStandbyAnchor = MakeSlot(
        center - entranceDir * 10.0f - state.sideDir * 3.0f,
        zAverage);

    state.initialized = true;

    sLog.outDetail(
        "[EncounterAI][Maulgar][Formation] frame captured "
        "center=(%.2f,%.2f) entrance=(%.3f,%.3f) "
        "source=COUNCIL_PLUS_RAID_CENTROID",
        center.x,
        center.y,
        entranceDir.x,
        entranceDir.y);

    return true;
}

bool MaulgarFormationManager::MaintainPreparationPosition(
    PlayerbotAI* ai,
    MaulgarPreparationRole role)
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();
    Slot* slot = PreparationSlot(ai, bot, role);
    if (!slot)
        return false;

    const float tolerance = PreparationTolerance(role);
    if (AtSlot(bot, *slot, tolerance))
    {
        MoveTowardSlot(
            ai,
            *slot,
            tolerance,
            PREP_MOVE_ID_BASE + static_cast<uint32>(role),
            true);
        return true;
    }

    MoveTowardSlot(
        ai,
        *slot,
        tolerance,
        PREP_MOVE_ID_BASE + static_cast<uint32>(role),
        true);
    return false;
}

bool MaulgarFormationManager::IsPreparationActorReady(
    PlayerbotAI* ai,
    Player* actor,
    MaulgarPreparationRole role)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !actor || !actor->IsAlive() ||
        actor->GetMap() != state->map)
    {
        return false;
    }

    if (IsProtectedHuman(actor))
    {
        if (role == MaulgarPreparationRole::KroshMage ||
            role == MaulgarPreparationRole::OlmWarlock)
        {
            return HumanRangeReady(*state, actor, role);
        }

        return true;
    }

    Slot* slot = PreparationSlot(ai, actor, role);
    return slot && AtSlot(actor, *slot, PreparationTolerance(role));
}

const char* MaulgarFormationManager::PreparationRoleName(
    MaulgarPreparationRole role)
{
    switch (role)
    {
        case MaulgarPreparationRole::MaulgarTank:
            return "MAULGAR_TANK";
        case MaulgarPreparationRole::BlindeyeTank:
            return "BLINDEYE_TANK";
        case MaulgarPreparationRole::KigglerTank:
            return "KIGGLER_TANK";
        case MaulgarPreparationRole::KroshMage:
            return "KROSH_MAGE";
        case MaulgarPreparationRole::OlmWarlock:
            return "OLM_WARLOCK";
        case MaulgarPreparationRole::FelhunterStandby:
            return "FELHUNTER_STANDBY";
        case MaulgarPreparationRole::HunterMaulgar:
            return "HUNTER_MAULGAR";
        case MaulgarPreparationRole::HunterBlindeye:
            return "HUNTER_BLINDEYE";
        case MaulgarPreparationRole::HunterKiggler:
            return "HUNTER_KIGGLER";
        case MaulgarPreparationRole::HealerBackline:
            return "HEALER_BACKLINE";
        case MaulgarPreparationRole::RangedBackline:
            return "RANGED_BACKLINE";
        case MaulgarPreparationRole::MeleeBackline:
            return "MELEE_BACKLINE";
        default:
            return "UNKNOWN";
    }
}

bool MaulgarFormationManager::EnsureTankAnchor(
    PlayerbotAI* ai,
    Creature* target,
    MaulgarTankAnchorRole role)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !ai->GetBot())
        return false;

    Slot anchor;
    switch (role)
    {
        case MaulgarTankAnchorRole::Maulgar:
            anchor = state->maulgarAnchor;
            break;
        case MaulgarTankAnchorRole::Blindeye:
            anchor = state->blindeyeAnchor;
            break;
        case MaulgarTankAnchorRole::Olm:
            anchor = state->olmAnchor;
            break;
        case MaulgarTankAnchorRole::FelhunterStandby:
            anchor = state->felStandbyAnchor;
            break;
        default:
            return false;
    }

    if (target && target->IsAlive())
    {
        anchor.o = ai->GetBot()->GetAngle(target);
    }

    return MoveTowardSlot(
        ai,
        anchor,
        TANK_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 10u + static_cast<uint32>(role),
        false);
}

bool MaulgarFormationManager::EnsureKroshMagePosition(
    PlayerbotAI* ai,
    Creature* krosh)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !krosh || !krosh->IsAlive())
        return false;

    return MoveTowardSlot(
        ai,
        RelativeTargetSlot(*state, krosh, KROSH_MAGE_RANGE, 0.0f),
        RANGED_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 30,
        false);
}

bool MaulgarFormationManager::EnsureKigglerPosition(
    PlayerbotAI* ai,
    Creature* kiggler)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !kiggler || !kiggler->IsAlive())
        return false;

    return MoveTowardSlot(
        ai,
        RelativeTargetSlot(*state, kiggler, KIGGLER_TANK_RANGE, 0.0f),
        RANGED_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 31,
        false);
}

bool MaulgarFormationManager::EnsureOlmWarlockPosition(
    PlayerbotAI* ai,
    Creature* olm)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !olm || !olm->IsAlive())
        return false;

    return MoveTowardSlot(
        ai,
        RelativeTargetSlot(*state, olm, OLM_WARLOCK_RANGE, 0.0f),
        RANGED_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 32,
        false);
}

bool MaulgarFormationManager::EnsureHealerPosition(PlayerbotAI* ai)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !ai->GetBot())
        return false;

    const uint32 ordinal = GroupOrdinal(ai);
    const int32 lane = static_cast<int32>(ordinal % 7) - 3;
    const Vec2 position =
        state->councilCenter +
        state->entranceDir * HEALER_BACKLINE_DISTANCE +
        state->sideDir * (static_cast<float>(lane) * 3.5f);

    return MoveTowardSlot(
        ai,
        MakeSlot(position, ai->GetBot()->GetPositionZ()),
        HEALER_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 40 + (ordinal % 16),
        false);
}

bool MaulgarFormationManager::EnsureRangedPosition(
    PlayerbotAI* ai,
    Unit* target)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !target || !target->IsAlive())
        return false;

    const uint32 ordinal = GroupOrdinal(ai);
    const int32 lane = static_cast<int32>(ordinal % 9) - 4;
    const float lateral = static_cast<float>(lane) * 2.25f;

    return MoveTowardSlot(
        ai,
        RelativeTargetSlot(*state, target, RANGED_TARGET_RANGE, lateral),
        RANGED_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 60 + (ordinal % 24),
        false);
}

bool MaulgarFormationManager::EnsureMeleePosition(
    PlayerbotAI* ai,
    Unit* target)
{
    if (!ai || !ai->GetBot() || !target || !target->IsAlive())
        return false;

    const uint32 ordinal = GroupOrdinal(ai);
    const int32 lane = static_cast<int32>(ordinal % 5) - 2;
    const float angle =
        target->GetOrientation() +
        M_PI_F +
        static_cast<float>(lane) * 0.18f;

    float x = 0.0f;
    float y = 0.0f;
    target->GetNearPoint2d(x, y, MELEE_REAR_RANGE, angle);

    return MoveTowardSlot(
        ai,
        Slot(x, y, target->GetPositionZ(), ai->GetBot()->GetAngle(target)),
        MELEE_MOVE_TOLERANCE,
        FORMATION_MOVE_ID_BASE + 90 + (ordinal % 24),
        false);
}

bool MaulgarFormationManager::HandleMaulgarWhirlwind(
    PlayerbotAI* ai,
    Creature* maulgar)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !ai->GetBot() || !maulgar || !maulgar->IsAlive())
        return false;

    Player* bot = ai->GetBot();
    if (bot->IsWithinDist3d(
            maulgar->GetPositionX(),
            maulgar->GetPositionY(),
            maulgar->GetPositionZ(),
            WHIRLWIND_SAFE_RANGE))
    {
        Vec2 away = Position2(bot) - Position2(maulgar);
        if (Length(away) < 1.0f)
            away = state->entranceDir;

        away = Normalize(away);
        const Vec2 escape =
            Position2(maulgar) + away * WHIRLWIND_ESCAPE_RANGE;

        MoveTowardSlot(
            ai,
            MakeSlot(escape, bot->GetPositionZ()),
            2.0f,
            FORMATION_MOVE_ID_BASE + 120 + (GroupOrdinal(ai) % 24),
            false);
    }

    return true;
}

bool MaulgarFormationManager::IsMeleeFormationActor(uint32 guid)
{
    switch (guid)
    {
        // Melee DPS
        case 17: case 22: case 37: case 32: case 30: case 175:
        case 39: case 56: case 63: case 53: case 46: case 387:
        case 83: case 12: case 103: case 126: case 174: case 400:

        // Protection Paladin transient pickup / standby actors
        case 31: case 97: case 98:
            return true;

        default:
            return false;
    }
}
