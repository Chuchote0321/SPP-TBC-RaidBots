#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Internal.h"

#include "playerbot/PlayerbotAI.h"

#include "AI/ScriptDevAI/include/sc_grid_searchers.h"
#include "Util/Timer.h"

#include <map>

using namespace ai;
using namespace ai::karazhan_phase1_detail;

namespace
{
    constexpr float SEARCH_RANGE = 120.0f;
    constexpr uint32 NPC_ATTUMEN_MOUNTED = 16152;
    constexpr float ATTUMEN_ASSIST_SEPARATION = 10.0f;
    constexpr float ATTUMEN_TANK_OFFSET = 7.0f;
    constexpr float ATTUMEN_MELEE_STACK = 2.5f;
    constexpr float ATTUMEN_HUNTER_STACK = 8.0f;
    constexpr uint32 ATTUMEN_DPS_WAIT_MS = 5000;
    constexpr uint32 MOVE_ID_BASE = 0x4B415200; // "KAR\0"

    struct AttumenState
    {
        AttumenState()
            : mountedGuid(), mountedSeenMs(0), center() {}

        ObjectGuid mountedGuid;
        uint32 mountedSeenMs;
        Slot center;
    };

    std::map<Map*, AttumenState> s_attumen;

    AttumenState* State(PlayerbotAI* ai, bool create = true)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;

        Map* map = ai->GetBot()->GetMap();
        if (create)
            return &s_attumen[map];

        auto itr = s_attumen.find(map);
        return itr == s_attumen.end() ? nullptr : &itr->second;
    }
}

Unit* KarazhanPhase1Runtime::FindMountedAttumen(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    return GetClosestCreatureWithEntry(
        ai->GetBot(),
        NPC_ATTUMEN_MOUNTED,
        SEARCH_RANGE,
        true);
}

bool KarazhanPhase1Runtime::IsAttumenPhaseOne(PlayerbotAI* ai)
{
    Unit* midnight = FindTarget(ai, "midnight");
    return midnight && midnight->IsAlive() &&
           !FindMountedAttumen(ai);
}

bool KarazhanPhase1Runtime::IsAttumenPhaseTwo(PlayerbotAI* ai)
{
    // CMaNGOS despawns the separate Midnight and unmounted Attumen actors
    // after spawning entry 16152. Unlike the AzerothCore implementation,
    // phase two therefore cannot require an invisible Midnight threat entry.
    return FindMountedAttumen(ai);
}

bool KarazhanPhase1Runtime::HandleAttumenPhaseOne(PlayerbotAI* ai)
{
    Unit* midnight = FindTarget(ai, "midnight");
    if (!ai || !ai->GetBot() || !midnight || FindMountedAttumen(ai))
        return false;

    AttumenState* state = State(ai);
    if (state && !state->center.valid)
        state->center = RespawnCenter(midnight);

    Unit* attumen = FindTarget(ai, "attumen the huntsman");
    if (IsAssistTank(ai) && attumen && attumen->IsAlive())
    {
        SetEncounterTarget(ai, attumen);

        if (attumen->GetVictim() != ai->GetBot() ||
            midnight->GetVictim() == ai->GetBot())
        {
            return false;
        }

        const Vec2 raidCenter = RaidCentroid(ai);
        const Vec2 outward = Normalize(Position2(attumen) - raidCenter);
        Slot slot(
            attumen->GetPositionX() +
                outward.x * ATTUMEN_ASSIST_SEPARATION,
            attumen->GetPositionY() +
                outward.y * ATTUMEN_ASSIST_SEPARATION,
            attumen->GetPositionZ());

        if (!CandidateValid(
                ai->GetBot(), attumen, slot, 6.0f, 14.0f))
        {
            return false;
        }

        return MoveToward(ai, slot, 2.5f, MOVE_ID_BASE + 1);
    }

    if (!PlayerbotAI::IsHeal(ai->GetBot(), false))
        SetEncounterTarget(ai, midnight);

    return false;
}

bool KarazhanPhase1Runtime::ObserveAttumenTransition(
    PlayerbotAI* ai)
{
    Unit* mounted = FindMountedAttumen(ai);
    AttumenState* state = State(ai);
    if (!mounted || !state)
        return false;

    if (state->mountedGuid == mounted->GetObjectGuid() &&
        state->mountedSeenMs)
    {
        return false;
    }

    state->mountedGuid = mounted->GetObjectGuid();
    state->mountedSeenMs = WorldTimer::getMSTime();
    if (!state->center.valid)
        state->center = RespawnCenter(mounted);

    return false;
}

bool KarazhanPhase1Runtime::HandleAttumenPhaseTwo(PlayerbotAI* ai)
{
    Unit* mounted = FindMountedAttumen(ai);
    if (!ai || !ai->GetBot() || !mounted)
        return false;

    ObserveAttumenTransition(ai);
    SetEncounterTarget(ai, mounted);

    AttumenState* state = State(ai);
    if (!state)
        return false;

    if (IsMainTank(ai) && mounted->GetVictim() == ai->GetBot())
    {
        Slot slot = FarSideTankSlot(
            ai, mounted, state->center, ATTUMEN_TANK_OFFSET);
        return MoveToward(ai, slot, 2.5f, MOVE_ID_BASE + 2);
    }

    // Upstream exempts every tank from the rear-stack movement override.
    if (PlayerbotAI::IsTank(ai->GetBot(), false) ||
        PlayerbotAI::IsHeal(ai->GetBot(), false))
    {
        return false;
    }

    const float stackDistance =
        ai->GetBot()->getClass() == CLASS_HUNTER
            ? ATTUMEN_HUNTER_STACK
            : ATTUMEN_MELEE_STACK;

    Slot slot = BehindTargetSlot(ai, mounted, stackDistance);
    return MoveToward(
        ai,
        slot,
        1.75f,
        MOVE_ID_BASE + 3 +
            (ai->GetBot()->GetObjectGuid().GetCounter() % 32));
}

bool KarazhanPhase1Runtime::ShouldSuppressAttumenAutomaticTargeting(
    PlayerbotAI* ai)
{
    return IsAttumenPhaseOne(ai) || IsAttumenPhaseTwo(ai);
}

bool KarazhanPhase1Runtime::ShouldKeepAttumenStacked(
    PlayerbotAI* ai)
{
    return ai && ai->GetBot() &&
           IsAttumenPhaseTwo(ai) &&
           !PlayerbotAI::IsTank(ai->GetBot(), false) &&
           !PlayerbotAI::IsHeal(ai->GetBot(), false);
}

bool KarazhanPhase1Runtime::ShouldWaitForAttumenTank(
    PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() ||
        !IsAttumenPhaseTwo(ai) ||
        IsMainTank(ai) ||
        PlayerbotAI::IsHeal(ai->GetBot(), false))
    {
        return false;
    }

    ObserveAttumenTransition(ai);
    AttumenState* state = State(ai, false);
    return state && state->mountedSeenMs &&
           WorldTimer::getMSTime() - state->mountedSeenMs <
               ATTUMEN_DPS_WAIT_MS;
}

void ai::karazhan_phase1_detail::ResetAttumen(PlayerbotAI* ai)
{
    if (ai && ai->GetBot() && ai->GetBot()->GetMap())
        s_attumen.erase(ai->GetBot()->GetMap());
}
