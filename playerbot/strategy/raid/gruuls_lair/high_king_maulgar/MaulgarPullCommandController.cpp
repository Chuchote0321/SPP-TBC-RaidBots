#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandController.h"

#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCoordinator.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFormationManager.h"
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"

#include "AI/ScriptDevAI/include/sc_grid_searchers.h"
#include "Maps/InstanceData.h"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>

using namespace ai;

namespace
{
    constexpr float SEARCH_RANGE = 220.0f;
    constexpr uint32 MISDIRECTION = 34477;

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

    // Raid1 / Raid2 / Raid3. Keep aligned with the existing Maulgar roster.
    const RaidPullRoster ROSTERS[3] =
    {
        { 15, 145, 21, 31, 20, 35, {27, 94, 235}, {4, 71, 58} },
        { 24, 100, 50, 97, 40, 43, {29, 109, 236}, {25, 6, 70} },
        { 88, 124, 99, 98, 114, 72, {74, 169, 243}, {36, 48, 75} }
    };

    const uint32 HUMAN_MAGE_TANKS[] = { 4504, 4506 }; // Game, Migu
    const uint32 HUMAN_OLM_WARLOCKS[] = { 4503 };     // Chuchote

    struct PullState
    {
        PullState()
            : phase(MaulgarPullCommandPhase::Idle),
              rosterIndex(-1),
              prepareAnnounced(false),
              positioningAnnounced(false),
              positionsReadyAnnounced(false),
              humanPositionAnnounced(false),
              armedAnnounced(false),
              pullAnnounced(false),
              blockedAnnounced(false),
              pullGo(false),
              warlockOpened(false),
              observedInProgress(false)
        {
            for (uint8 lane = 0;
                 lane < static_cast<uint8>(HunterPullLane::Count);
                 ++lane)
            {
                hunterOpened[lane] = false;
            }
        }

        MaulgarPullCommandPhase phase;
        int8 rosterIndex;
        bool prepareAnnounced;
        bool positioningAnnounced;
        bool positionsReadyAnnounced;
        bool humanPositionAnnounced;
        bool armedAnnounced;
        bool pullAnnounced;
        bool blockedAnnounced;
        bool pullGo;
        bool warlockOpened;
        bool hunterOpened[3];
        bool observedInProgress;
    };

    std::map<Map*, PullState> s_states;

    PullState* GetState(PlayerbotAI* ai, bool create = true)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;

        Map* map = ai->GetBot()->GetMap();
        if (create)
            return &s_states[map];

        auto itr = s_states.find(map);
        return itr == s_states.end() ? nullptr : &itr->second;
    }

    void ClearOpening(PullState& state)
    {
        state.pullGo = false;
        state.warlockOpened = false;
        state.blockedAnnounced = false;

        for (uint8 lane = 0;
             lane < static_cast<uint8>(HunterPullLane::Count);
             ++lane)
        {
            state.hunterOpened[lane] = false;
        }
    }

    Creature* FindCreature(PlayerbotAI* ai, uint32 entry)
    {
        return ai && ai->GetBot()
            ? GetClosestCreatureWithEntry(
                  ai->GetBot(), entry, SEARCH_RANGE, true)
            : nullptr;
    }

    EncounterActor FindHuman(
        PlayerbotAI* ai,
        const uint32* guids,
        size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            EncounterActor actor =
                EncounterActorResolver::Find(ai, guids[i]);

            if (actor.IsValid() && actor.IsHuman())
                return actor;
        }

        return EncounterActor();
    }

    EncounterActor FirstBot(
        PlayerbotAI* ai,
        const uint32* guids,
        size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            EncounterActor actor =
                EncounterActorResolver::Find(ai, guids[i]);

            if (actor.IsValid() && actor.IsBot())
                return actor;
        }

        return EncounterActor();
    }

    EncounterActor ResolveMage(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        EncounterActor human = FindHuman(
            ai,
            HUMAN_MAGE_TANKS,
            sizeof(HUMAN_MAGE_TANKS) / sizeof(uint32));

        if (human.IsValid())
            return human;

        const uint32 candidates[2] =
        {
            roster.mageArcane,
            roster.mageFire
        };

        return FirstBot(ai, candidates, 2);
    }

    EncounterActor ResolveWarlock(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        EncounterActor human = FindHuman(
            ai,
            HUMAN_OLM_WARLOCKS,
            sizeof(HUMAN_OLM_WARLOCKS) / sizeof(uint32));

        if (human.IsValid())
            return human;

        return FirstBot(ai, roster.warlocks, 3);
    }

    uint32 PresenceScore(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        const uint32 guids[] =
        {
            roster.feralTank,
            roster.protWarTank,
            roster.balanceTank,
            roster.protPalTank,
            roster.mageArcane,
            roster.mageFire,
            roster.warlocks[0],
            roster.warlocks[1],
            roster.warlocks[2],
            roster.hunters[0],
            roster.hunters[1],
            roster.hunters[2]
        };

        uint32 score = 0;
        for (uint32 guid : guids)
        {
            if (EncounterActorResolver::Find(ai, guid).IsValid())
                ++score;
        }

        return score;
    }

    int8 DetectRoster(PlayerbotAI* ai)
    {
        int8 bestIndex = -1;
        uint32 bestScore = 0;

        for (int8 index = 0; index < 3; ++index)
        {
            const uint32 score = PresenceScore(ai, ROSTERS[index]);
            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = index;
            }
        }

        return bestScore ? bestIndex : -1;
    }

    RaidPullRoster const* ResolveRoster(
        PlayerbotAI* ai,
        PullState& state)
    {
        if (state.rosterIndex < 0 || state.rosterIndex > 2)
            state.rosterIndex = DetectRoster(ai);

        return state.rosterIndex >= 0 && state.rosterIndex <= 2
            ? &ROSTERS[state.rosterIndex]
            : nullptr;
    }

    EncounterActor Hunter(
        PlayerbotAI* ai,
        RaidPullRoster const& roster,
        uint8 lane)
    {
        return lane < static_cast<uint8>(HunterPullLane::Count)
            ? EncounterActorResolver::Find(ai, roster.hunters[lane])
            : EncounterActor();
    }

    EncounterActor MisdirectionRecipient(
        PlayerbotAI* ai,
        RaidPullRoster const& roster,
        uint8 lane)
    {
        switch (static_cast<HunterPullLane>(lane))
        {
            case HunterPullLane::Maulgar:
                return EncounterActorResolver::Find(ai, roster.feralTank);
            case HunterPullLane::Blindeye:
                return EncounterActorResolver::Find(ai, roster.protWarTank);
            case HunterPullLane::Kiggler:
                return EncounterActorResolver::Find(ai, roster.balanceTank);
            default:
                return EncounterActor();
        }
    }

    Creature* HunterTarget(PlayerbotAI* ai, uint8 lane)
    {
        switch (static_cast<HunterPullLane>(lane))
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

    bool RedirectsTo(Player* hunter, Player* recipient)
    {
        if (!hunter || !recipient)
            return false;

        Unit* target =
            hunter->getHostileRefManager().GetThreatRedirectionTarget();

        return target &&
               target->GetObjectGuid() == recipient->GetObjectGuid();
    }

    bool CouncilPresent(PlayerbotAI* ai)
    {
        return
            FindCreature(ai, EncounterConstants::NPC_MAULGAR) &&
            FindCreature(ai, EncounterConstants::NPC_KROSH) &&
            FindCreature(ai, EncounterConstants::NPC_OLM) &&
            FindCreature(ai, EncounterConstants::NPC_KIGGLER) &&
            FindCreature(ai, EncounterConstants::NPC_BLINDEYE);
    }

    bool PullActorsPresent(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        if (!CouncilPresent(ai) ||
            !EncounterActorResolver::Find(ai, roster.feralTank).IsValid() ||
            !EncounterActorResolver::Find(ai, roster.protWarTank).IsValid() ||
            !EncounterActorResolver::Find(ai, roster.balanceTank).IsValid() ||
            !EncounterActorResolver::Find(ai, roster.protPalTank).IsValid() ||
            !ResolveMage(ai, roster).IsValid() ||
            !ResolveWarlock(ai, roster).IsValid())
        {
            return false;
        }

        for (uint8 lane = 0;
             lane < static_cast<uint8>(HunterPullLane::Count);
             ++lane)
        {
            if (!Hunter(ai, roster, lane).IsValid())
                return false;
        }

        return true;
    }

    bool IsHumanPlayer(Player* player)
    {
        if (!player)
            return false;

        PlayerbotAI* playerAI = player->GetPlayerbotAI();
        return !playerAI || playerAI->IsRealPlayer();
    }

    MaulgarPreparationRole PreparationRoleFor(
        PlayerbotAI* ai,
        RaidPullRoster const& roster,
        Player* player)
    {
        if (!ai || !player)
            return MaulgarPreparationRole::MeleeBackline;

        const uint32 guid = player->GetObjectGuid().GetCounter();
        if (guid == roster.feralTank)
            return MaulgarPreparationRole::MaulgarTank;
        if (guid == roster.protWarTank)
            return MaulgarPreparationRole::BlindeyeTank;
        if (guid == roster.balanceTank)
            return MaulgarPreparationRole::KigglerTank;
        if (guid == roster.protPalTank)
            return MaulgarPreparationRole::FelhunterStandby;

        EncounterActor mage = ResolveMage(ai, roster);
        if (mage.IsValid() && mage.LowGuid() == guid)
            return MaulgarPreparationRole::KroshMage;

        EncounterActor warlock = ResolveWarlock(ai, roster);
        if (warlock.IsValid() && warlock.LowGuid() == guid)
            return MaulgarPreparationRole::OlmWarlock;

        if (guid == roster.hunters[0])
            return MaulgarPreparationRole::HunterMaulgar;
        if (guid == roster.hunters[1])
            return MaulgarPreparationRole::HunterBlindeye;
        if (guid == roster.hunters[2])
            return MaulgarPreparationRole::HunterKiggler;

        if (PlayerbotAI::IsHeal(player, false))
            return MaulgarPreparationRole::HealerBackline;
        if (ai->IsRanged(player, false))
            return MaulgarPreparationRole::RangedBackline;

        return MaulgarPreparationRole::MeleeBackline;
    }

    bool EnsurePreparationFrame(PlayerbotAI* ai)
    {
        Creature* maulgar =
            FindCreature(ai, EncounterConstants::NPC_MAULGAR);
        Creature* krosh =
            FindCreature(ai, EncounterConstants::NPC_KROSH);
        Creature* olm =
            FindCreature(ai, EncounterConstants::NPC_OLM);
        Creature* kiggler =
            FindCreature(ai, EncounterConstants::NPC_KIGGLER);
        Creature* blindeye =
            FindCreature(ai, EncounterConstants::NPC_BLINDEYE);

        return MaulgarFormationManager::EnsureMaulgarFrame(
            ai,
            maulgar,
            krosh,
            olm,
            kiggler,
            blindeye);
    }

    bool AllFormationReady(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        if (!ai || !ai->GetBot())
            return false;

        const EncounterActor mage = ResolveMage(ai, roster);
        const EncounterActor warlock = ResolveWarlock(ai, roster);

        for (Player* member : ai->GetPlayersInGroup())
        {
            if (!member || !member->IsAlive() ||
                member->GetMap() != ai->GetBot()->GetMap())
            {
                continue;
            }

            const uint32 guid = member->GetObjectGuid().GetCounter();
            const bool criticalHuman =
                (mage.IsValid() && mage.IsHuman() &&
                 mage.LowGuid() == guid) ||
                (warlock.IsValid() && warlock.IsHuman() &&
                 warlock.LowGuid() == guid);

            // Real players are never server-positioned. Only the protected
            // human Mage/Warlock assignments participate in the safe-range
            // readiness barrier; unrelated real players are ignored.
            if (IsHumanPlayer(member) && !criticalHuman)
                continue;

            const MaulgarPreparationRole role =
                PreparationRoleFor(ai, roster, member);

            if (!MaulgarFormationManager::IsPreparationActorReady(
                    ai,
                    member,
                    role))
            {
                return false;
            }
        }

        return true;
    }

    std::string HumanPositionPendingMessage(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        std::ostringstream details;
        bool pending = false;

        const EncounterActor mage = ResolveMage(ai, roster);
        if (mage.IsValid() && mage.IsHuman() &&
            !MaulgarFormationManager::IsPreparationActorReady(
                ai,
                mage.player,
                MaulgarPreparationRole::KroshMage))
        {
            pending = true;
            details << " KROSH_MAGE='20-32yd LOS'";
        }

        const EncounterActor warlock = ResolveWarlock(ai, roster);
        if (warlock.IsValid() && warlock.IsHuman() &&
            !MaulgarFormationManager::IsPreparationActorReady(
                ai,
                warlock.player,
                MaulgarPreparationRole::OlmWarlock))
        {
            pending = true;
            details << " OLM_WARLOCK='18-32yd LOS'";
        }

        return pending
            ? "MAULGAR_HUMAN_POSITION_REQUIRED" + details.str()
            : std::string();
    }

    bool AllMisdirectionsReady(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        for (uint8 lane = 0;
             lane < static_cast<uint8>(HunterPullLane::Count);
             ++lane)
        {
            EncounterActor hunter = Hunter(ai, roster, lane);
            EncounterActor recipient =
                MisdirectionRecipient(ai, roster, lane);

            if (!hunter.IsValid() ||
                !recipient.IsValid() ||
                !RedirectsTo(hunter.player, recipient.player))
            {
                return false;
            }
        }

        return true;
    }

    bool AllHuntersOpened(PullState const& state)
    {
        for (uint8 lane = 0;
             lane < static_cast<uint8>(HunterPullLane::Count);
             ++lane)
        {
            if (!state.hunterOpened[lane])
                return false;
        }

        return true;
    }

    bool IsResponder(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot())
            return false;

        uint32 lowest = std::numeric_limits<uint32>::max();

        for (Player* player : ai->GetPlayersInGroup())
        {
            if (!player || !player->IsAlive() || !ai->IsSafe(player))
                continue;

            PlayerbotAI* playerAI = player->GetPlayerbotAI();
            if (!playerAI || playerAI->IsRealPlayer())
                continue;

            lowest = std::min(
                lowest,
                player->GetObjectGuid().GetCounter());
        }

        return lowest != std::numeric_limits<uint32>::max() &&
               lowest == ai->GetBot()->GetObjectGuid().GetCounter();
    }

    void TellRequester(
        PlayerbotAI* ai,
        Player* requester,
        std::string const& text)
    {
        if (ai && requester && IsResponder(ai))
            ai->TellPlayer(requester, text);
    }

    void Announce(
        PlayerbotAI* ai,
        bool& alreadyAnnounced,
        std::string const& text)
    {
        if (!alreadyAnnounced && IsResponder(ai))
        {
            alreadyAnnounced = true;
            ai->SayToRaid(text);
        }
    }

    bool Authorized(
        PlayerbotAI* ai,
        Player* requester,
        std::string& reason)
    {
        if (!ai || !ai->GetBot() || !requester)
        {
            reason = "missing requester";
            return false;
        }

        Player* bot = ai->GetBot();
        Group* group = bot->GetGroup();

        if (!group || !group->IsRaidGroup())
        {
            reason = "bot is not in a raid group";
            return false;
        }

        if (requester->GetGroup() != group)
        {
            reason = "requester is not in this raid";
            return false;
        }

        if (!requester->IsInWorld() ||
            requester->GetMap() != bot->GetMap())
        {
            reason = "requester is not in this Gruul's Lair instance";
            return false;
        }

        const ObjectGuid guid = requester->GetObjectGuid();
        if (!group->IsLeader(guid) && !group->IsAssistant(guid))
        {
            reason = "requester is not raid leader or assistant";
            return false;
        }

        if (bot->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
        {
            reason = "bot is not in Gruul's Lair";
            return false;
        }

        InstanceData* instance =
            bot->GetMap() ? bot->GetMap()->GetInstanceData() : nullptr;

        if (!instance)
        {
            reason = "instance data is unavailable";
            return false;
        }

        if (instance->GetData(EncounterConstants::TYPE_MAULGAR_EVENT) != 0)
        {
            reason = "High King Maulgar is not in NOT_STARTED state";
            return false;
        }

        return true;
    }

    void SetTarget(PlayerbotAI* ai, Unit* target)
    {
        if (!ai || !target || !target->IsAlive())
            return;

        AiObjectContext* context = ai->GetAiObjectContext();
        if (!context)
            return;

        context->GetValue<Unit*>("current target")->Set(target);
        context->GetValue<ObjectGuid>("attack target")
            ->Set(target->GetObjectGuid());
    }

    EncounterOverrideResult ArmCurrentHunter(
        PlayerbotAI* ai,
        RaidPullRoster const& roster)
    {
        const uint32 self = ai->GetBot()->GetObjectGuid().GetCounter();

        for (uint8 lane = 0;
             lane < static_cast<uint8>(HunterPullLane::Count);
             ++lane)
        {
            EncounterActor hunter = Hunter(ai, roster, lane);
            if (!hunter.IsValid() ||
                !hunter.IsBot() ||
                hunter.LowGuid() != self)
            {
                continue;
            }

            EncounterActor recipient =
                MisdirectionRecipient(ai, roster, lane);

            if (!recipient.IsValid())
                return EncounterOverrideResult::BlockNormal;

            if (RedirectsTo(ai->GetBot(), recipient.player))
                return EncounterOverrideResult::BlockNormal;

            if (!ai->HasSpell(MISDIRECTION))
            {
                sLog.outError(
                    "[EncounterAI][Maulgar][Command] Hunter %s guid=%u "
                    "does not know Misdirection 34477",
                    ai->GetBot()->GetName(),
                    self);
                return EncounterOverrideResult::BlockNormal;
            }

            if (ai->CastSpell(MISDIRECTION, recipient.player))
            {
                EncounterTrace::Event(
                    ai,
                    "MAULGAR",
                    "COMMAND_MD_ARM",
                    "lane=%u hunter=%s hunterGuid=%u recipient=%s "
                    "recipientGuid=%u",
                    lane,
                    ai->GetBot()->GetName(),
                    self,
                    recipient.player->GetName(),
                    recipient.LowGuid());

                return EncounterOverrideResult::Handled;
            }

            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::NotHandled;
    }

    EncounterOverrideResult StartBotMage(
        PlayerbotAI* ai,
        PullState& state,
        RaidPullRoster const& roster)
    {
        EncounterActor mage = ResolveMage(ai, roster);
        if (!mage.IsValid() || mage.IsHuman() ||
            !EncounterActorResolver::IsCurrentBot(ai, mage))
        {
            return EncounterOverrideResult::NotHandled;
        }

        Creature* krosh =
            FindCreature(ai, EncounterConstants::NPC_KROSH);

        if (!krosh)
            return EncounterOverrideResult::BlockNormal;

        SetTarget(ai, krosh);

        if (ai->HasSpell("frostbolt") &&
            ai->CastSpell("frostbolt", krosh))
        {
            state.pullGo = true;

            EncounterTrace::Event(
                ai,
                "MAULGAR",
                "COMMAND_PULL_GO",
                "source=BOT_MAGE_FROSTBOLT mage=%s mageGuid=%u "
                "target=KROSH targetGuid=%u",
                ai->GetBot()->GetName(),
                mage.LowGuid(),
                krosh->GetObjectGuid().GetCounter());

            return EncounterOverrideResult::Handled;
        }

        return EncounterOverrideResult::BlockNormal;
    }

    EncounterOverrideResult StartCurrentHunter(
        PlayerbotAI* ai,
        PullState& state,
        RaidPullRoster const& roster)
    {
        const uint32 self = ai->GetBot()->GetObjectGuid().GetCounter();

        for (uint8 lane = 0;
             lane < static_cast<uint8>(HunterPullLane::Count);
             ++lane)
        {
            EncounterActor hunter = Hunter(ai, roster, lane);
            if (!hunter.IsValid() ||
                !hunter.IsBot() ||
                hunter.LowGuid() != self)
            {
                continue;
            }

            if (state.hunterOpened[lane])
                return EncounterOverrideResult::NotHandled;

            EncounterActor recipient =
                MisdirectionRecipient(ai, roster, lane);
            Creature* target = HunterTarget(ai, lane);

            if (!recipient.IsValid() || !target)
                return EncounterOverrideResult::BlockNormal;

            // If this lane lost its short Misdirection window while another
            // actor was starting the pull, re-arm only this pending lane.
            if (!RedirectsTo(ai->GetBot(), recipient.player))
            {
                if (ai->HasSpell(MISDIRECTION) &&
                    ai->CastSpell(MISDIRECTION, recipient.player))
                {
                    EncounterTrace::Event(
                        ai,
                        "MAULGAR",
                        "COMMAND_MD_REARM",
                        "lane=%u hunter=%s hunterGuid=%u recipient=%s "
                        "recipientGuid=%u",
                        lane,
                        ai->GetBot()->GetName(),
                        self,
                        recipient.player->GetName(),
                        recipient.LowGuid());

                    return EncounterOverrideResult::Handled;
                }

                return EncounterOverrideResult::BlockNormal;
            }

            SetTarget(ai, target);

            bool started = false;
            if (ai->HasSpell("auto shot"))
            {
                started = ai->CastSpell(
                    "auto shot", target, nullptr, false) || started;
            }

            if (ai->HasSpell("arcane shot"))
                started = ai->CastSpell("arcane shot", target) || started;

            if (!started && ai->HasSpell("steady shot"))
                started = ai->CastSpell("steady shot", target) || started;

            if (started)
            {
                state.hunterOpened[lane] = true;

                EncounterTrace::Event(
                    ai,
                    "MAULGAR",
                    "COMMAND_HUNTER_OPEN",
                    "lane=%u hunter=%s hunterGuid=%u target=%s "
                    "targetGuid=%u recipient=%s recipientGuid=%u",
                    lane,
                    ai->GetBot()->GetName(),
                    self,
                    target->GetName(),
                    target->GetObjectGuid().GetCounter(),
                    recipient.player->GetName(),
                    recipient.LowGuid());

                return EncounterOverrideResult::Handled;
            }

            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::NotHandled;
    }

    EncounterOverrideResult StartBotWarlock(
        PlayerbotAI* ai,
        PullState& state,
        RaidPullRoster const& roster)
    {
        if (state.warlockOpened)
            return EncounterOverrideResult::NotHandled;

        EncounterActor warlock = ResolveWarlock(ai, roster);
        if (!warlock.IsValid() || warlock.IsHuman() ||
            !EncounterActorResolver::IsCurrentBot(ai, warlock))
        {
            return EncounterOverrideResult::NotHandled;
        }

        Creature* olm =
            FindCreature(ai, EncounterConstants::NPC_OLM);

        if (!olm)
            return EncounterOverrideResult::BlockNormal;

        SetTarget(ai, olm);

        if (ai->HasSpell("searing pain") &&
            ai->CastSpell("searing pain", olm))
        {
            state.warlockOpened = true;

            EncounterTrace::Event(
                ai,
                "MAULGAR",
                "COMMAND_OLM_OPEN",
                "warlock=%s warlockGuid=%u target=OLM targetGuid=%u",
                ai->GetBot()->GetName(),
                warlock.LowGuid(),
                olm->GetObjectGuid().GetCounter());

            return EncounterOverrideResult::Handled;
        }

        return EncounterOverrideResult::BlockNormal;
    }

    std::string PullRequestedMessage(
        EncounterActor const& mage,
        EncounterActor const& warlock)
    {
        std::ostringstream out;
        out << "MAULGAR_PULL_REQUESTED THREE_MD=READY";

        if (mage.IsHuman())
            out << " HUMAN_KROSH_FROSTBOLT=REQUIRED_BEFORE_HUNTERS";
        else
            out << " BOT_KROSH_FROSTBOLT=AUTO";

        if (warlock.IsHuman())
            out << " HUMAN_OLM_SEARING_PAIN=REQUIRED";
        else
            out << " BOT_OLM_SEARING_PAIN=AUTO";

        return out.str();
    }
}

const char* MaulgarPullCommandController::PhaseName(
    MaulgarPullCommandPhase phase)
{
    switch (phase)
    {
        case MaulgarPullCommandPhase::Idle:
            return "IDLE";
        case MaulgarPullCommandPhase::Preparing:
            return "PREPARING";
        case MaulgarPullCommandPhase::Armed:
            return "ARMED";
        case MaulgarPullCommandPhase::PullRequested:
            return "PULL_REQUESTED";
        case MaulgarPullCommandPhase::InProgress:
            return "IN_PROGRESS";
        case MaulgarPullCommandPhase::Complete:
            return "COMPLETE";
        default:
            return "UNKNOWN";
    }
}

MaulgarPullCommandPhase MaulgarPullCommandController::GetPhase(
    PlayerbotAI* ai)
{
    PullState* state = GetState(ai, false);
    return state ? state->phase : MaulgarPullCommandPhase::Idle;
}

void MaulgarPullCommandController::Reset(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return;

    s_states.erase(ai->GetBot()->GetMap());
    MaulgarFormationManager::Reset(ai);
    MaulgarPullCoordinator::Reset(ai);
}

bool MaulgarPullCommandController::RequestPrepare(
    PlayerbotAI* ai,
    Player* requester)
{
    std::string reason;
    if (!Authorized(ai, requester, reason))
    {
        TellRequester(
            ai,
            requester,
            "MAULGAR_PREPARE_REJECTED reason='" + reason + "'");
        return false;
    }

    PullState* state = GetState(ai);
    if (!state)
        return false;

    if (state->phase != MaulgarPullCommandPhase::Preparing &&
        state->phase != MaulgarPullCommandPhase::Armed)
    {
        *state = PullState();
        state->phase = MaulgarPullCommandPhase::Preparing;
        state->rosterIndex = DetectRoster(ai);

        // Explicit commands are the sole owners of NOT_STARTED. A
        // fresh prepare captures a new encounter-local frame.
        MaulgarPullCoordinator::Reset(ai);
        MaulgarFormationManager::Reset(ai);
    }

    Announce(
        ai,
        state->prepareAnnounced,
        "MAULGAR_PREPARE_ACCEPTED POSITION_MODE=AUTO_RELATIVE "
        "FRAME=COUNCIL_PLUS_RAID_CENTROID "
        "WAIT_FOR=MAULGAR_POSITIONING_READY");

    return true;
}

bool MaulgarPullCommandController::RequestPull(
    PlayerbotAI* ai,
    Player* requester)
{
    std::string reason;
    if (!Authorized(ai, requester, reason))
    {
        TellRequester(
            ai,
            requester,
            "MAULGAR_PULL_REJECTED reason='" + reason + "'");
        return false;
    }

    PullState* state = GetState(ai);
    if (!state)
        return false;

    RaidPullRoster const* roster = ResolveRoster(ai, *state);
    const bool duplicate =
        state->phase == MaulgarPullCommandPhase::PullRequested;

    if (!roster ||
        (!duplicate && state->phase != MaulgarPullCommandPhase::Armed) ||
        (!duplicate &&
         (!PullActorsPresent(ai, *roster) ||
          !AllFormationReady(ai, *roster) ||
          !AllMisdirectionsReady(ai, *roster))))
    {
        std::ostringstream out;
        out << "MAULGAR_PULL_REJECTED phase="
            << PhaseName(state->phase)
            << " required='raid prepare maulgar'";
        TellRequester(ai, requester, out.str());
        return false;
    }

    if (!duplicate)
    {
        state->phase = MaulgarPullCommandPhase::PullRequested;
        state->pullAnnounced = false;
        ClearOpening(*state);
    }

    Announce(
        ai,
        state->pullAnnounced,
        PullRequestedMessage(
            ResolveMage(ai, *roster),
            ResolveWarlock(ai, *roster)));

    return true;
}

EncounterOverrideResult MaulgarPullCommandController::Update(
    PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() ||
        ai->GetBot()->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
    {
        return EncounterOverrideResult::NotHandled;
    }

    Player* bot = ai->GetBot();
    InstanceData* instance =
        bot->GetMap() ? bot->GetMap()->GetInstanceData() : nullptr;

    if (!instance)
        return EncounterOverrideResult::NotHandled;

    PullState* state = GetState(ai, false);
    const uint32 encounterState =
        instance->GetData(EncounterConstants::TYPE_MAULGAR_EVENT);
    const bool coreInProgress =
        encounterState == EncounterConstants::ENCOUNTER_IN_PROGRESS;

    if (encounterState == EncounterConstants::ENCOUNTER_DONE)
    {
        if (state)
            state->phase = MaulgarPullCommandPhase::Complete;

        return EncounterOverrideResult::NotHandled;
    }

    if (encounterState != 0 && !coreInProgress)
    {
        Reset(ai);
        return EncounterOverrideResult::NotHandled;
    }

    if (!state)
        return EncounterOverrideResult::NotHandled;

    // A wipe returning from IN_PROGRESS to NOT_STARTED needs a fresh prepare.
    if (!coreInProgress &&
        (state->phase == MaulgarPullCommandPhase::InProgress ||
         state->observedInProgress ||
         state->phase == MaulgarPullCommandPhase::Complete))
    {
        Reset(ai);
        return EncounterOverrideResult::NotHandled;
    }

    if (state->phase == MaulgarPullCommandPhase::Idle)
        return EncounterOverrideResult::NotHandled;

    RaidPullRoster const* roster = ResolveRoster(ai, *state);
    if (!roster || !PullActorsPresent(ai, *roster))
    {
        Announce(
            ai,
            state->blockedAnnounced,
            "MAULGAR_PREPARE_BLOCKED "
            "reason='required actor or council member missing'");
        return coreInProgress
            ? EncounterOverrideResult::NotHandled
            : EncounterOverrideResult::BlockNormal;
    }

    if (coreInProgress)
        state->observedInProgress = true;

    if (!coreInProgress &&
        (state->phase == MaulgarPullCommandPhase::Preparing ||
         state->phase == MaulgarPullCommandPhase::Armed))
    {
        if (!EnsurePreparationFrame(ai))
        {
            Announce(
                ai,
                state->blockedAnnounced,
                "MAULGAR_PREPARE_BLOCKED "
                "reason='dynamic formation frame unavailable'");
            return EncounterOverrideResult::BlockNormal;
        }

        state->blockedAnnounced = false;

        Announce(
            ai,
            state->positioningAnnounced,
            "MAULGAR_POSITIONING_AUTO "
            "FRAME=COUNCIL_PLUS_RAID_CENTROID "
            "MOVE=ROLE_RELATIVE_5YD_STEPS "
            "VALIDATION=GROUND_LOS_PATHFINDER");

        const MaulgarPreparationRole currentRole =
            PreparationRoleFor(ai, *roster, bot);
        MaulgarFormationManager::MaintainPreparationPosition(
            ai,
            currentRole);

        if (!AllFormationReady(ai, *roster))
        {
            state->phase = MaulgarPullCommandPhase::Preparing;
            state->armedAnnounced = false;
            state->positionsReadyAnnounced = false;

            const std::string humanPending =
                HumanPositionPendingMessage(ai, *roster);
            if (!humanPending.empty())
            {
                Announce(
                    ai,
                    state->humanPositionAnnounced,
                    humanPending);
            }
            else
            {
                state->humanPositionAnnounced = false;
            }

            return EncounterOverrideResult::BlockNormal;
        }

        state->humanPositionAnnounced = false;
        Announce(
            ai,
            state->positionsReadyAnnounced,
            "MAULGAR_POSITIONING_READY "
            "BOTS=AUTO_POSITIONED HUMAN_SPECIALISTS=SAFE_RANGE");

        EncounterOverrideResult armResult =
            ArmCurrentHunter(ai, *roster);

        if (armResult == EncounterOverrideResult::Handled)
            return armResult;

        if (AllMisdirectionsReady(ai, *roster))
        {
            state->phase = MaulgarPullCommandPhase::Armed;
            Announce(
                ai,
                state->armedAnnounced,
                "MAULGAR_PULL_ARMED "
                "POSITIONING=READY "
                "MD_MAULGAR=READY "
                "MD_BLINDEYE=READY "
                "MD_KIGGLER=READY "
                "KROSH_MAGE=READY "
                "OLM_WARLOCK=READY "
                "COMMAND='raid pull maulgar'");
        }
        else
        {
            state->phase = MaulgarPullCommandPhase::Preparing;
            state->armedAnnounced = false;
        }

        return EncounterOverrideResult::BlockNormal;
    }

    if (state->phase != MaulgarPullCommandPhase::PullRequested)
    {
        if (coreInProgress)
        {
            state->phase = MaulgarPullCommandPhase::InProgress;
            return EncounterOverrideResult::NotHandled;
        }

        return EncounterOverrideResult::NotHandled;
    }

    // Hold the completed formation while releasing only the
    // synchronized Mage/Hunter/Warlock opening.
    bot->StopMoving();

    EncounterActor mage = ResolveMage(ai, *roster);

    if (!state->pullGo)
    {
        if (!coreInProgress && !AllMisdirectionsReady(ai, *roster))
        {
            state->phase = MaulgarPullCommandPhase::Preparing;
            state->armedAnnounced = false;
            ClearOpening(*state);

            Announce(
                ai,
                state->blockedAnnounced,
                "MAULGAR_PULL_ABORTED "
                "reason='Misdirection expired; run raid prepare maulgar again'");
            return EncounterOverrideResult::BlockNormal;
        }

        if (mage.IsValid() && mage.IsHuman())
        {
            Creature* krosh =
                FindCreature(ai, EncounterConstants::NPC_KROSH);

            // A protected human is never cast-controlled. Hunters stay held
            // until Krosh is actually engaged by that human Mage.
            if (coreInProgress ||
                (krosh && (krosh->IsInCombat() || krosh->GetVictim())))
            {
                state->pullGo = true;

                EncounterTrace::Event(
                    ai,
                    "MAULGAR",
                    "COMMAND_PULL_GO",
                    "source=HUMAN_MAGE_ENGAGED mage=%s mageGuid=%u "
                    "target=KROSH targetGuid=%u",
                    mage.player->GetName(),
                    mage.LowGuid(),
                    krosh ? krosh->GetObjectGuid().GetCounter() : 0);
            }
            else
            {
                return EncounterOverrideResult::BlockNormal;
            }
        }
        else
        {
            EncounterOverrideResult mageResult =
                StartBotMage(ai, *state, *roster);

            if (mageResult != EncounterOverrideResult::NotHandled)
                return mageResult;

            // An unexpected external pull must not deadlock the raid.
            if (coreInProgress)
                state->pullGo = true;
        }

        if (!state->pullGo)
            return EncounterOverrideResult::BlockNormal;
    }

    EncounterOverrideResult hunterResult =
        StartCurrentHunter(ai, *state, *roster);

    if (hunterResult != EncounterOverrideResult::NotHandled)
        return hunterResult;

    EncounterOverrideResult warlockResult =
        StartBotWarlock(ai, *state, *roster);

    if (warlockResult != EncounterOverrideResult::NotHandled)
        return warlockResult;

    const EncounterActor warlock = ResolveWarlock(ai, *roster);
    const bool warlockOpeningComplete =
        !warlock.IsValid() || warlock.IsHuman() || state->warlockOpened;

    if (coreInProgress &&
        AllHuntersOpened(*state) &&
        warlockOpeningComplete)
    {
        state->phase = MaulgarPullCommandPhase::InProgress;
        return EncounterOverrideResult::NotHandled;
    }

    // Hold the rest of the raid until the synchronized opening is complete.
    return EncounterOverrideResult::BlockNormal;
}

bool MaulgarPullCommandController::AllowEncounterDispatch(
    PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() ||
        ai->GetBot()->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
    {
        return true;
    }

    InstanceData* instance =
        ai->GetBot()->GetMap()
            ? ai->GetBot()->GetMap()->GetInstanceData()
            : nullptr;

    if (!instance)
        return true;

    const uint32 maulgarState =
        instance->GetData(EncounterConstants::TYPE_MAULGAR_EVENT);

    // The legacy NOT_STARTED auto-pull path is never reachable.
    return maulgarState != 0;
}
