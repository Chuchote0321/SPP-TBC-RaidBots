#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCoordinator.h"

#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFixedPositions.h"
#include "playerbot/PlayerbotAI.h"

#include "AI/ScriptDevAI/include/sc_grid_searchers.h"
#include "Maps/InstanceData.h"
#include "playerbot/strategy/AiObjectContext.h"

#include <map>

using namespace ai;

namespace
{
    constexpr float SEARCH_RANGE = 220.0f;
    constexpr float READY_TOLERANCE = 1.25f;
    constexpr uint32 MISDIRECTION = 34477;
    constexpr uint32 MOVE_ID_BASE = 0x4D503600; // "MP6\0"

    enum class HunterPullLane : uint8
    {
        Maulgar = 0,
        Blindeye = 1,
        Kiggler = 2,
        Count = 3
    };

    struct RaidPullRoster
    {
        uint32 feralTank;
        uint32 protWarTank;
        uint32 balanceTank;
        uint32 protPalTank;

        uint32 mageArcane;
        uint32 mageFire;

        uint32 warlocks[3];
        uint32 hunters[3];
    };

    // Raid1 / Raid2 / Raid3. Hunter composition follows the established
    // progression roster:
    //   Raid1 = BMone + BMfour + Survivalone
    //   Raid2 = BMtwo + BMfive + Survivaltwo
    //   Raid3 = BMthree + BMsix + Survivalthr
    const RaidPullRoster ROSTERS[3] =
    {
        { 15, 145, 21, 31, 20, 35, {27, 94, 235},  {4, 71, 58} },
        { 24, 100, 50, 97, 40, 43, {29, 109, 236}, {25, 6, 70} },
        { 88, 124, 99, 98, 114,72, {74, 169,243},  {36,48,75} }
    };

    const uint32 HUMAN_MAGE_TANKS[] = { 4504, 4506 }; // Game, Migu
    const uint32 HUMAN_OLM_WARLOCKS[] = { 4503 };     // Chuchote

    struct PullState
    {
        PullState()
            : raidIndex(-1),
              magePullIssued(false),
              warlockPullIssued(false),
              combatObserved(false),
              configErrorLogged(false)
        {
            for (uint8 i = 0; i < 3; ++i)
            {
                hunterMdReady[i] = false;
                hunterOpeningComplete[i] = false;
            }
        }

        int8 raidIndex;
        bool magePullIssued;
        bool warlockPullIssued;
        bool combatObserved;
        bool configErrorLogged;
        bool hunterMdReady[3];
        bool hunterOpeningComplete[3];
    };

    std::map<Map*, PullState> s_pullState;

    PullState* State(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;

        return &s_pullState[ai->GetBot()->GetMap()];
    }

    Creature* FindCreature(PlayerbotAI* ai, uint32 entry)
    {
        if (!ai || !ai->GetBot())
            return nullptr;

        return GetClosestCreatureWithEntry(
            ai->GetBot(), entry, SEARCH_RANGE, true);
    }

    void SetTarget(PlayerbotAI* ai, Unit* target)
    {
        if (!ai || !target || !target->IsAlive())
            return;

        AiObjectContext* context = ai->GetAiObjectContext();
        if (!context)
            return;

        context->GetValue<Unit*>("current target")->Set(target);
        context->GetValue<ObjectGuid>("attack target")->Set(target->GetObjectGuid());
    }

    EncounterActor FindHuman(PlayerbotAI* ai, const uint32* guids, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            EncounterActor actor = EncounterActorResolver::Find(ai, guids[i]);
            if (actor.IsValid() && actor.IsHuman())
                return actor;
        }

        return EncounterActor();
    }

    EncounterActor FindBotOrHuman(PlayerbotAI* ai, uint32 lowGuid)
    {
        return EncounterActorResolver::Find(ai, lowGuid);
    }

    EncounterActor FirstBot(PlayerbotAI* ai, const uint32* guids, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            EncounterActor actor = EncounterActorResolver::Find(ai, guids[i]);
            if (actor.IsValid() && actor.IsBot())
                return actor;
        }

        return EncounterActor();
    }

    int8 DetectRaid(PlayerbotAI* ai)
    {
        for (int8 i = 0; i < 3; ++i)
        {
            EncounterActor feral =
                EncounterActorResolver::Find(ai, ROSTERS[i].feralTank);

            if (feral.IsValid())
                return i;
        }

        return -1;
    }

    MaulgarFixedAnchor HunterAnchor(uint8 lane)
    {
        switch (HunterPullLane(lane))
        {
            case HunterPullLane::Maulgar:
                return MaulgarFixedPositions::HunterToMaulgar();
            case HunterPullLane::Blindeye:
                return MaulgarFixedPositions::HunterToBlindeye();
            case HunterPullLane::Kiggler:
                return MaulgarFixedPositions::HunterToKiggler();
            default:
                return MaulgarFixedAnchor();
        }
    }

    Unit* HunterMisdirectionTarget(
        PlayerbotAI* ai,
        RaidPullRoster const& roster,
        uint8 lane)
    {
        uint32 guid = 0;

        switch (HunterPullLane(lane))
        {
            case HunterPullLane::Maulgar:
                guid = roster.feralTank;
                break;
            case HunterPullLane::Blindeye:
                guid = roster.protWarTank;
                break;
            case HunterPullLane::Kiggler:
                guid = roster.balanceTank;
                break;
            default:
                return nullptr;
        }

        EncounterActor actor = EncounterActorResolver::Find(ai, guid);
        return actor.IsValid() ? actor.player : nullptr;
    }

    Creature* HunterPullTarget(PlayerbotAI* ai, uint8 lane)
    {
        switch (HunterPullLane(lane))
        {
            case HunterPullLane::Maulgar:
                return FindCreature(ai, EncounterConstants::NPC_MAULGAR);
            case HunterPullLane::Blindeye:
                return FindCreature(ai, EncounterConstants::NPC_BLINDEYE);
            case HunterPullLane::Kiggler:
                return FindCreature(ai, EncounterConstants::NPC_KIGGLER);
            default:
                return nullptr;
        }
    }

    EncounterActor ResolveMageTank(PlayerbotAI* ai, RaidPullRoster const& roster)
    {
        EncounterActor human =
            FindHuman(ai, HUMAN_MAGE_TANKS, sizeof(HUMAN_MAGE_TANKS)/sizeof(uint32));

        if (human.IsValid())
            return human;

        uint32 candidates[2] = { roster.mageArcane, roster.mageFire };
        return FirstBot(ai, candidates, 2);
    }

    EncounterActor ResolveOlmWarlock(PlayerbotAI* ai, RaidPullRoster const& roster)
    {
        EncounterActor human =
            FindHuman(ai, HUMAN_OLM_WARLOCKS, sizeof(HUMAN_OLM_WARLOCKS)/sizeof(uint32));

        if (human.IsValid())
            return human;

        return FirstBot(ai, roster.warlocks, 3);
    }

    bool AtAnchor(Player* player, MaulgarFixedAnchor const& anchor)
    {
        if (!player || !anchor.configured)
            return false;

        return player->IsWithinDist3d(
            anchor.x, anchor.y, anchor.z, READY_TOLERANCE);
    }

    bool HoldCurrentActorAtAnchor(
        PlayerbotAI* ai,
        EncounterActor const& actor,
        MaulgarFixedAnchor const& anchor,
        uint32 moveId)
    {
        if (!ai || !ai->GetBot() || !actor.IsValid() || !anchor.configured)
            return false;

        // Human actors must be positioned manually. Readiness still requires
        // them to actually be on the same absolute anchor.
        if (actor.IsHuman())
            return AtAnchor(actor.player, anchor);

        if (!EncounterActorResolver::IsCurrentBot(ai, actor))
            return AtAnchor(actor.player, anchor);

        Player* bot = ai->GetBot();

        if (!AtAnchor(bot, anchor))
        {
            bot->GetMotionMaster()->MovePoint(
                MOVE_ID_BASE + moveId,
                anchor.x, anchor.y, anchor.z,
                FORCED_MOVEMENT_RUN,
                true);
            return false;
        }

        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() !=
            STAY_MOTION_TYPE)
        {
            bot->GetMotionMaster()->MoveStay(
                anchor.x, anchor.y, anchor.z, anchor.o, true);
        }

        return true;
    }

    bool ActorReady(
        EncounterActor const& actor,
        MaulgarFixedAnchor const& anchor)
    {
        return actor.IsValid() && AtAnchor(actor.player, anchor);
    }

    bool ThreatRedirectsTo(Player* hunter, Unit* tank)
    {
        if (!hunter || !tank)
            return false;

        Unit* target =
            hunter->getHostileRefManager().GetThreatRedirectionTarget();

        return target && target->GetObjectGuid() == tank->GetObjectGuid();
    }

    bool AllFixedActorsReady(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        EncounterActor feral =
            EncounterActorResolver::Find(ai, roster.feralTank);
        EncounterActor protWar =
            EncounterActorResolver::Find(ai, roster.protWarTank);
        EncounterActor balance =
            EncounterActorResolver::Find(ai, roster.balanceTank);
        EncounterActor protPal =
            EncounterActorResolver::Find(ai, roster.protPalTank);
        EncounterActor mage = ResolveMageTank(ai, roster);
        EncounterActor warlock = ResolveOlmWarlock(ai, roster);

        if (!ActorReady(feral, MaulgarFixedPositions::MaulgarTank()) ||
            !ActorReady(protWar, MaulgarFixedPositions::BlindeyeTank()) ||
            !ActorReady(balance, MaulgarFixedPositions::KigglerTank()) ||
            !ActorReady(protPal, MaulgarFixedPositions::FelhunterPaladinStandby()) ||
            !ActorReady(mage, MaulgarFixedPositions::KroshMageTank()) ||
            !ActorReady(warlock, MaulgarFixedPositions::OlmWarlockTank()))
        {
            return false;
        }

        for (uint8 lane = 0; lane < 3; ++lane)
        {
            EncounterActor hunter =
                EncounterActorResolver::Find(ai, roster.hunters[lane]);

            if (!ActorReady(hunter, HunterAnchor(lane)))
                return false;
        }

        return true;
    }

    bool AllHunterMdReady(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        for (uint8 lane = 0; lane < 3; ++lane)
        {
            EncounterActor hunter =
                EncounterActorResolver::Find(ai, roster.hunters[lane]);

            Unit* tank = HunterMisdirectionTarget(ai, roster, lane);

            if (!hunter.IsValid() || !tank ||
                !ThreatRedirectsTo(hunter.player, tank))
            {
                return false;
            }
        }

        return true;
    }

    // Pre-pull absolute positioning for only the designated encounter actors.
    void MaintainPrePullAnchors(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        EncounterActor feral =
            EncounterActorResolver::Find(ai, roster.feralTank);
        EncounterActor protWar =
            EncounterActorResolver::Find(ai, roster.protWarTank);
        EncounterActor balance =
            EncounterActorResolver::Find(ai, roster.balanceTank);
        EncounterActor protPal =
            EncounterActorResolver::Find(ai, roster.protPalTank);
        EncounterActor mage = ResolveMageTank(ai, roster);
        EncounterActor warlock = ResolveOlmWarlock(ai, roster);

        HoldCurrentActorAtAnchor(
            ai, feral, MaulgarFixedPositions::MaulgarTank(), 1);
        HoldCurrentActorAtAnchor(
            ai, protWar, MaulgarFixedPositions::BlindeyeTank(), 2);
        HoldCurrentActorAtAnchor(
            ai, balance, MaulgarFixedPositions::KigglerTank(), 3);
        HoldCurrentActorAtAnchor(
            ai, mage, MaulgarFixedPositions::KroshMageTank(), 4);
        HoldCurrentActorAtAnchor(
            ai, warlock, MaulgarFixedPositions::OlmWarlockTank(), 5);
        HoldCurrentActorAtAnchor(
            ai, protPal, MaulgarFixedPositions::FelhunterPaladinStandby(), 6);

        for (uint8 lane = 0; lane < 3; ++lane)
        {
            EncounterActor hunter =
                EncounterActorResolver::Find(ai, roster.hunters[lane]);

            HoldCurrentActorAtAnchor(
                ai, hunter, HunterAnchor(lane), 10 + lane);
        }
    }

    EncounterOverrideResult PrepareCurrentHunterMisdirection(
        PlayerbotAI* ai,
        PullState& state,
        RaidPullRoster const& roster)
    {
        if (!ai || !ai->GetBot())
            return EncounterOverrideResult::NotHandled;

        const uint32 self = ai->GetBot()->GetObjectGuid().GetCounter();

        for (uint8 lane = 0; lane < 3; ++lane)
        {
            if (self != roster.hunters[lane])
                continue;

            Unit* tank = HunterMisdirectionTarget(ai, roster, lane);
            if (!tank)
                return EncounterOverrideResult::BlockNormal;

            if (!AtAnchor(ai->GetBot(), HunterAnchor(lane)))
                return EncounterOverrideResult::BlockNormal;

            if (ThreatRedirectsTo(ai->GetBot(), tank))
            {
                state.hunterMdReady[lane] = true;
                return EncounterOverrideResult::BlockNormal;
            }

            state.hunterMdReady[lane] = false;

            if (!ai->HasSpell(MISDIRECTION))
            {
                sLog.outError(
                    "[EncounterAI][Maulgar][Pull] Hunter %s guid=%u does not know Misdirection 34477",
                    ai->GetBot()->GetName(),
                    self);
                return EncounterOverrideResult::BlockNormal;
            }

            if (ai->CastSpell(MISDIRECTION, tank))
            {
                state.hunterMdReady[lane] = true;

                sLog.outDetail(
                    "[EncounterAI][Maulgar][Pull] Hunter %s guid=%u Misdirection -> %s guid=%u lane=%u",
                    ai->GetBot()->GetName(),
                    self,
                    tank->GetName(),
                    tank->GetObjectGuid().GetCounter(),
                    lane);

                EncounterTrace::Event(
                    ai,
                    "MAULGAR",
                    "MISDIRECT_CAST",
                    "mode=PREPULL hunter=%s hunterGuid=%u target=%s targetGuid=%u lane=%u",
                    ai->GetBot()->GetName(),
                    self,
                    tank->GetName(),
                    tank->GetObjectGuid().GetCounter(),
                    lane);

                return EncounterOverrideResult::Handled;
            }

            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::NotHandled;
    }
}

bool MaulgarPullCoordinator::IsConfigured()
{
    return MaulgarFixedPositions::PullAnchorsConfigured();
}

void MaulgarPullCoordinator::Reset(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return;

    s_pullState.erase(ai->GetBot()->GetMap());
}

EncounterOverrideResult MaulgarPullCoordinator::UpdatePrePull(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return EncounterOverrideResult::NotHandled;

    PullState* state = State(ai);
    if (!state)
        return EncounterOverrideResult::NotHandled;

    if (!IsConfigured())
    {
        if (!state->configErrorLogged)
        {
            state->configErrorLogged = true;
            sLog.outError(
                "[EncounterAI][Maulgar][Pull] fixed pull anchors are NOT configured; "
                "auto-position/auto-pull disabled: WCL anchor profile is not populated");
        }

        return EncounterOverrideResult::NotHandled;
    }

    if (state->raidIndex < 0)
        state->raidIndex = DetectRaid(ai);

    if (state->raidIndex < 0 || state->raidIndex > 2)
        return EncounterOverrideResult::NotHandled;

    RaidPullRoster const& roster = ROSTERS[state->raidIndex];

    Creature* krosh =
        FindCreature(ai, EncounterConstants::NPC_KROSH);
    Creature* olm =
        FindCreature(ai, EncounterConstants::NPC_OLM);

    if (!krosh || !krosh->IsAlive())
        return EncounterOverrideResult::NotHandled;

    MaintainPrePullAnchors(ai, roster);

    const uint32 self = ai->GetBot()->GetObjectGuid().GetCounter();
    const bool allActorsReady = AllFixedActorsReady(ai, roster);

    EncounterActor mage = ResolveMageTank(ai, roster);
    EncounterActor olmWarlock = ResolveOlmWarlock(ai, roster);

    EncounterActor feral =
        EncounterActorResolver::Find(ai, roster.feralTank);
    EncounterActor protWar =
        EncounterActorResolver::Find(ai, roster.protWarTank);
    EncounterActor balance =
        EncounterActorResolver::Find(ai, roster.balanceTank);
    EncounterActor protPal =
        EncounterActorResolver::Find(ai, roster.protPalTank);

    EncounterTrace::Assignment(ai, "MAULGAR", "MAULGAR_MT", feral);
    EncounterTrace::Assignment(ai, "MAULGAR", "BLINDEYE_TANK", protWar);
    EncounterTrace::Assignment(ai, "MAULGAR", "KIGGLER_TANK", balance);
    EncounterTrace::Assignment(ai, "MAULGAR", "FELHUNTER_PALADIN", protPal);
    EncounterTrace::Assignment(ai, "MAULGAR", "KROSH_CONTROLLER", mage);
    EncounterTrace::Assignment(ai, "MAULGAR", "OLM_WARLOCK", olmWarlock);

    EncounterTrace::ProtectedHuman(ai, "MAULGAR", "KROSH_CONTROLLER", mage);
    EncounterTrace::ProtectedHuman(ai, "MAULGAR", "OLM_WARLOCK", olmWarlock);

    for (uint8 traceLane = 0; traceLane < 3; ++traceLane)
    {
        EncounterActor hunter =
            EncounterActorResolver::Find(ai, roster.hunters[traceLane]);

        char role[32];
        snprintf(role, sizeof(role), "HUNTER_MD_%u", traceLane);
        EncounterTrace::Assignment(ai, "MAULGAR", role, hunter);
    }

    // ------------------------------------------------------------------
    // HUMAN MAGE TANK
    //
    // Do NOT pre-cast Misdirection here. Server AI cannot know when the
    // human will actually pull, and burning the 30-second MD window early
    // is unsafe. Hunters are held at their fixed lanes; on the first
    // IN_PROGRESS update they immediately MD + open their assigned target.
    // ------------------------------------------------------------------
    if (mage.IsValid() && mage.IsHuman())
    {
        EncounterTrace::EventOnce(
            ai,
            "MAULGAR",
            "human-mage-wait",
            "PULL_ARMED",
            "mode=HUMAN_MAGE_WAIT actor=%s actorGuid=%u huntersHeld=1",
            mage.player ? mage.player->GetName() : "UNKNOWN",
            mage.LowGuid());

        return EncounterOverrideResult::BlockNormal;
    }

    // ------------------------------------------------------------------
    // BOT MAGE TANK
    //
    // Misdirection is PREPARED before the pull, but the three Hunter
    // ATTACKS are released only after the Mage begins the Krosh pull.
    // ------------------------------------------------------------------
    EncounterOverrideResult md =
        PrepareCurrentHunterMisdirection(ai, *state, roster);

    if (md != EncounterOverrideResult::NotHandled)
        return md;

    const bool mdReady = AllHunterMdReady(ai, roster);

    if (!allActorsReady || !mdReady)
        return EncounterOverrideResult::BlockNormal;

    EncounterTrace::EventOnce(
        ai,
        "MAULGAR",
        "bot-pull-armed",
        "PULL_ARMED",
        "mode=BOT_MAGE raidIndex=%d allActorsReady=1 mdReady=1 mageGuid=%u",
        int(state->raidIndex),
        mage.LowGuid());

    if (!mage.IsValid() || !mage.IsBot())
        return EncounterOverrideResult::BlockNormal;

    // Mage is the pull coordinator. Frostbolt cast-start defines PULL_GO.
    if (EncounterActorResolver::IsCurrentBot(ai, mage) &&
        !state->magePullIssued)
    {
        SetTarget(ai, krosh);

        if (ai->HasSpell("frostbolt") &&
            ai->CastSpell("frostbolt", krosh))
        {
            state->magePullIssued = true;

            sLog.outDetail(
                "[EncounterAI][Maulgar][Pull] PULL_GO Mage=%s guid=%u -> Krosh; "
                "release three Hunter MD lanes + Olm Warlock",
                ai->GetBot()->GetName(),
                self);

            EncounterTrace::Event(
                ai,
                "MAULGAR",
                "PULL_GO",
                "source=FROSTBOLT mage=%s mageGuid=%u target=KROSH targetGuid=%u raidIndex=%d",
                ai->GetBot()->GetName(),
                self,
                krosh->GetObjectGuid().GetCounter(),
                int(state->raidIndex));

            return EncounterOverrideResult::Handled;
        }

        return EncounterOverrideResult::BlockNormal;
    }

    // Before PULL_GO every bot remains frozen at the fixed pull setup.
    if (!state->magePullIssued)
        return EncounterOverrideResult::BlockNormal;

    // ------------------------------------------------------------------
    // SAME PULL EPOCH: THREE HUNTERS OPEN
    //
    // This is intentionally allowed while the instance may still report
    // NOT_STARTED. The Mage has already begun Frostbolt; the Hunters now
    // fire on their three fixed council targets without waiting for a later
    // IN_PROGRESS state transition.
    // ------------------------------------------------------------------
    if (self == roster.hunters[0] ||
        self == roster.hunters[1] ||
        self == roster.hunters[2])
    {
        EncounterOverrideResult hunterOpen = UpdateOpening(ai);

        // Hunters stay locked behind the opener until MD is consumed.
        if (hunterOpen != EncounterOverrideResult::NotHandled)
            return hunterOpen;

        // If a Hunter completed its opener unusually quickly, do not allow
        // generic pre-pull behavior while the instance is still NOT_STARTED.
        return EncounterOverrideResult::BlockNormal;
    }

    // ------------------------------------------------------------------
    // SAME PULL EPOCH: OLM WARLOCK OPENS
    // ------------------------------------------------------------------
    if (olmWarlock.IsValid() &&
        olmWarlock.IsBot() &&
        EncounterActorResolver::IsCurrentBot(ai, olmWarlock))
    {
        if (!state->warlockPullIssued && olm && olm->IsAlive())
        {
            SetTarget(ai, olm);

            if (ai->HasSpell("searing pain") &&
                ai->CastSpell("searing pain", olm))
            {
                state->warlockPullIssued = true;

                sLog.outDetail(
                    "[EncounterAI][Maulgar][Pull] Olm opener Warlock=%s guid=%u",
                    ai->GetBot()->GetName(),
                    self);

                EncounterTrace::Event(
                    ai,
                    "MAULGAR",
                    "OLM_OPENER",
                    "warlock=%s warlockGuid=%u target=OLM targetGuid=%u",
                    ai->GetBot()->GetName(),
                    self,
                    olm->GetObjectGuid().GetCounter());

                return EncounterOverrideResult::Handled;
            }

            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::BlockNormal;
    }

    // Mage remains committed to the Krosh pull while Frostbolt is being
    // cast/in flight and before the instance flips to IN_PROGRESS.
    if (EncounterActorResolver::IsCurrentBot(ai, mage))
        return EncounterOverrideResult::BlockNormal;

    // Tanks, healers and all other raid members stay at their waiting state
    // until the encounter actually enters IN_PROGRESS.
    return EncounterOverrideResult::BlockNormal;
}

EncounterOverrideResult MaulgarPullCoordinator::UpdateOpening(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return EncounterOverrideResult::NotHandled;

    PullState* state = State(ai);
    if (!state)
        return EncounterOverrideResult::NotHandled;

    if (state->raidIndex < 0)
        state->raidIndex = DetectRaid(ai);

    if (state->raidIndex < 0 || state->raidIndex > 2)
        return EncounterOverrideResult::NotHandled;

    RaidPullRoster const& roster = ROSTERS[state->raidIndex];
    const uint32 self = ai->GetBot()->GetObjectGuid().GetCounter();

    state->combatObserved = true;

    for (uint8 lane = 0; lane < 3; ++lane)
    {
        if (self != roster.hunters[lane])
            continue;

        if (state->hunterOpeningComplete[lane])
            return EncounterOverrideResult::NotHandled;

        Unit* tank = HunterMisdirectionTarget(ai, roster, lane);
        Creature* pullTarget = HunterPullTarget(ai, lane);

        if (!tank || !pullTarget || !pullTarget->IsAlive())
        {
            state->hunterOpeningComplete[lane] = true;
            return EncounterOverrideResult::NotHandled;
        }

        // Human Mage pull path: cast Misdirection immediately after the human
        // creates IN_PROGRESS. Bot-Mage path normally arrives here already armed.
        if (!ThreatRedirectsTo(ai->GetBot(), tank))
        {
            if (!state->hunterMdReady[lane] && ai->HasSpell(MISDIRECTION))
            {
                if (ai->CastSpell(MISDIRECTION, tank))
                {
                    state->hunterMdReady[lane] = true;

                    EncounterTrace::Event(
                        ai,
                        "MAULGAR",
                        "MISDIRECT_CAST",
                        "mode=HUMAN_PULL hunter=%s hunterGuid=%u target=%s targetGuid=%u lane=%u",
                        ai->GetBot()->GetName(),
                        self,
                        tank->GetName(),
                        tank->GetObjectGuid().GetCounter(),
                        lane);

                    return EncounterOverrideResult::Handled;
                }
            }
            else if (state->hunterMdReady[lane])
            {
                // Redirection was consumed/cleared: the three attacks are done.
                state->hunterOpeningComplete[lane] = true;

                sLog.outDetail(
                    "[EncounterAI][Maulgar][Pull] Hunter %s lane=%u MD opener complete",
                    ai->GetBot()->GetName(),
                    lane);

                EncounterTrace::Event(
                    ai,
                    "MAULGAR",
                    "MISDIRECT_CLEAR",
                    "hunter=%s hunterGuid=%u lane=%u openerComplete=1",
                    ai->GetBot()->GetName(),
                    self,
                    lane);

                return EncounterOverrideResult::NotHandled;
            }

            return EncounterOverrideResult::BlockNormal;
        }

        state->hunterMdReady[lane] = true;
        SetTarget(ai, pullTarget);

        // Start Auto Shot and inject Arcane Shot when available. While core
        // threat redirection is still active, this Hunter is locked to this
        // one assigned council member, guaranteeing all three Misdirection
        // attacks go to the same Tank rather than leaking to the raid kill order.
        if (ai->HasSpell("auto shot"))
            ai->CastSpell("auto shot", pullTarget, nullptr, false);

        if (ai->HasSpell("arcane shot") &&
            ai->CastSpell("arcane shot", pullTarget))
        {
            return EncounterOverrideResult::Handled;
        }

        if (ai->HasSpell("steady shot") &&
            ai->CastSpell("steady shot", pullTarget))
        {
            return EncounterOverrideResult::Handled;
        }

        // Auto Shot can continue while normal encounter target switching is
        // blocked. Next updates recheck the core redirection target; once the
        // three charges are consumed, the Hunter is released.
        return EncounterOverrideResult::BlockNormal;
    }

    return EncounterOverrideResult::NotHandled;
}
