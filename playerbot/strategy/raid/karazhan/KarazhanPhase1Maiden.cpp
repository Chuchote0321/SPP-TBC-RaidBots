#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Internal.h"

#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"

#include <cmath>
#include <map>

using namespace ai;
using namespace ai::karazhan_phase1_detail;

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr uint32 SPELL_REPENTANCE = 29511;
    constexpr float MAIDEN_TANK_OFFSET = 7.0f;
    constexpr float MAIDEN_HEALER_BREAK_OFFSET = 6.0f;
    constexpr uint32 MAIDEN_RANGED_SLOT_COUNT = 8;
    constexpr float MAIDEN_INNER_RING = 20.0f;
    constexpr float MAIDEN_OUTER_RING = 23.5f;
    constexpr uint32 MOVE_ID_BASE = 0x4B415200; // "KAR\0"

    struct MaidenState
    {
        Slot center;
        std::map<uint32, Slot> rangedSlots;
    };

    std::map<Map*, MaidenState> s_maiden;

    MaidenState* State(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;
        return &s_maiden[ai->GetBot()->GetMap()];
    }

    Player* RepentantHealer(PlayerbotAI* ai)
    {
        for (Player* player : SortedGroup(ai))
        {
            if (PlayerbotAI::IsHeal(player, false) &&
                player->HasAura(SPELL_REPENTANCE))
            {
                return player;
            }
        }

        return nullptr;
    }

    Slot ResolveRangedSlot(PlayerbotAI* ai, Unit* maiden)
    {
        MaidenState* state = State(ai);
        if (!state || !maiden)
            return Slot();

        if (!state->center.valid)
            state->center = RespawnCenter(maiden);

        const uint32 self =
            ai->GetBot()->GetObjectGuid().GetCounter();
        auto existing = state->rangedSlots.find(self);
        if (existing != state->rangedSlots.end())
            return existing->second;

        const uint32 ordinal = RangedOrdinal(ai, ai->GetBot());
        const uint32 lane = ordinal % MAIDEN_RANGED_SLOT_COUNT;
        const uint32 ring =
            (ordinal / MAIDEN_RANGED_SLOT_COUNT) % 2;
        const float radius =
            ring ? MAIDEN_OUTER_RING : MAIDEN_INNER_RING;
        const float laneAngle =
            TWO_PI * static_cast<float>(lane) /
            static_cast<float>(MAIDEN_RANGED_SLOT_COUNT);
        const float angle = ring
            ? laneAngle + PI /
                static_cast<float>(MAIDEN_RANGED_SLOT_COUNT)
            : laneAngle;

        static const float radiusOffsets[] =
            { 0.0f, 2.0f, -2.0f, 4.0f };
        static const float angleOffsets[] =
            { 0.0f, 0.10f, -0.10f, 0.20f, -0.20f };

        for (float radiusOffset : radiusOffsets)
        {
            for (float angleOffset : angleOffsets)
            {
                const float useRadius = radius + radiusOffset;
                const float useAngle = angle + angleOffset;
                Slot candidate(
                    state->center.x +
                        std::cos(useAngle) * useRadius,
                    state->center.y +
                        std::sin(useAngle) * useRadius,
                    state->center.z);

                if (CandidateValid(
                        ai->GetBot(), maiden, candidate, 16.0f, 30.0f))
                {
                    state->rangedSlots[self] = candidate;
                    return candidate;
                }
            }
        }

        return Slot();
    }
}

bool KarazhanPhase1Runtime::MaintainMaidenTankPosition(
    PlayerbotAI* ai)
{
    Unit* maiden = FindTarget(ai, "maiden of virtue");
    if (!ai || !ai->GetBot() || !maiden ||
        !PlayerbotAI::IsTank(ai->GetBot(), false))
    {
        return false;
    }

    SetEncounterTarget(ai, maiden);
    if (maiden->GetVictim() != ai->GetBot())
        return false;

    Slot slot;
    if (Player* healer = RepentantHealer(ai))
    {
        const Vec2 healerPosition = Position2(healer);
        const Vec2 beyond = Normalize(
            healerPosition - Position2(maiden));
        slot = Slot(
            healerPosition.x +
                beyond.x * MAIDEN_HEALER_BREAK_OFFSET,
            healerPosition.y +
                beyond.y * MAIDEN_HEALER_BREAK_OFFSET,
            healer->GetPositionZ());

        if (!CandidateValid(
                ai->GetBot(), maiden, slot, 0.0f, 0.0f))
        {
            return false;
        }
    }
    else
    {
        MaidenState* state = State(ai);
        if (!state)
            return false;

        if (!state->center.valid)
            state->center = RespawnCenter(maiden);

        slot = FarSideTankSlot(
            ai, maiden, state->center, MAIDEN_TANK_OFFSET);
    }

    return MoveToward(ai, slot, 2.5f, MOVE_ID_BASE + 50);
}

bool KarazhanPhase1Runtime::MaintainMaidenRangedPosition(
    PlayerbotAI* ai)
{
    Unit* maiden = FindTarget(ai, "maiden of virtue");
    if (!ai || !ai->GetBot() || !maiden ||
        !IsRangedOrHealer(ai) ||
        PlayerbotAI::IsTank(ai->GetBot(), false))
    {
        return false;
    }

    Slot slot = ResolveRangedSlot(ai, maiden);
    return MoveToward(
        ai,
        slot,
        2.5f,
        MOVE_ID_BASE + 60 +
            (RangedOrdinal(ai, ai->GetBot()) % 32));
}

bool KarazhanPhase1Runtime::CastMaidenGroundingTotem(
    PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() ||
        ai->GetBot()->getClass() != CLASS_SHAMAN ||
        !IsMaidenActive(ai))
    {
        return false;
    }

    AiObjectContext* context = ai->GetAiObjectContext();
    Value<bool>* hasTotem = context
        ? context->GetValue<bool>(
              "has totem", "grounding totem")
        : nullptr;

    if (hasTotem && hasTotem->Get())
        return false;

    return ai->HasSpell("grounding totem") &&
           ai->CastSpell("grounding totem", ai->GetBot());
}

bool KarazhanPhase1Runtime::ShouldSuppressMaidenFormation(
    PlayerbotAI* ai)
{
    return IsMaidenActive(ai);
}

bool KarazhanPhase1Runtime::ShouldReserveMaidenAirTotem(
    PlayerbotAI* ai)
{
    return ai && ai->GetBot() &&
           ai->GetBot()->getClass() == CLASS_SHAMAN &&
           IsMaidenActive(ai);
}

void ai::karazhan_phase1_detail::ResetMaiden(PlayerbotAI* ai)
{
    if (ai && ai->GetBot() && ai->GetBot()->GetMap())
        s_maiden.erase(ai->GetBot()->GetMap());
}
