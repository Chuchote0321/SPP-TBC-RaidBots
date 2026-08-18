#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/HighKingMaulgarEncounter.h"
#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFormationManager.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCoordinator.h"
#include "playerbot/PlayerbotAI.h"

#include "AI/ScriptDevAI/include/sc_grid_searchers.h"
#include "Maps/InstanceData.h"
#include "Spells/Spell.h"
#include "playerbot/strategy/AiObjectContext.h"

using namespace ai;

namespace
{
    constexpr float MAULGAR_SEARCH_RANGE = 220.0f;

    // Progression tanks.
    const uint32 MAULGAR_FERAL_TANKS[]      = { 15, 24, 88 };
    const uint32 BLINDEYE_WARRIOR_TANKS[]  = { 145, 100, 124 };
    const uint32 FELHUNTER_PALADIN_TANKS[] = { 31, 97, 98 };

    // Balance is preferred for Kiggler.
    const uint32 KIGGLER_BALANCE[] = { 21, 50, 99 };

    // Protected human Mage Tank actors.
    const uint32 HUMAN_MAGE_TANKS[] = { 4504, 4506 }; // Game, Migu

    // Automated Mage Tank fallback.
    const uint32 BOT_MAGE_TANKS[] = { 20, 40, 114, 35, 43, 72 };

    // Protected human Warlock special controller.
    const uint32 HUMAN_OLM_WARLOCKS[] = { 4503 }; // Chuchote

    // All progression Destruction warlocks. Only actors actually present in
    // the current group/instance resolve successfully.
    const uint32 BOT_OLM_WARLOCKS[] =
    {
        27, 94, 235,      // Raid1
        29, 109, 236,     // Raid2
        74, 169, 243      // Raid3
    };


    // ---------------------------------------------------------------------
    // Blindeye raid-level interrupt chain
    //
    // Round 1: all three Warriors in the active progression raid
    // Round 2: Combat Rogue + both Enhancement Shamans
    // Round 3: Arcane/Fire Mage + Elemental Shaman
    //
    // The state is keyed by the actual Map instance pointer, so every bot in
    // one raid shares one cast/round sequence instead of maintaining a local
    // per-bot counter.
    // ---------------------------------------------------------------------
    struct BlindeyeInterruptState
    {
        BlindeyeInterruptState()
            : activeSpell(nullptr), activeRound(0), nextRound(1), interrupted(false) {}

        Spell* activeSpell;
        uint8 activeRound;
        uint8 nextRound;
        bool interrupted;
    };

    std::map<Map*, BlindeyeInterruptState> s_blindeyeInterruptState;


    // ---------------------------------------------------------------------
    // Wild Fel Stalker control reservations.
    //
    // A reservation exists only while an uncontrolled Fel Stalker is waiting
    // for an automated Warlock to complete Enslave Demon. This prevents one
    // Warlock from being selected for a second newly-spawned Fel while the
    // first 3-second Enslave cast is still in progress.
    // ---------------------------------------------------------------------
    struct FelhunterControlReservation
    {
        ObjectGuid felGuid;
        uint32 warlockLowGuid;

        FelhunterControlReservation()
            : felGuid(), warlockLowGuid(0) {}

        FelhunterControlReservation(ObjectGuid const& fel, uint32 warlock)
            : felGuid(fel), warlockLowGuid(warlock) {}
    };

    std::map<Map*, std::vector<FelhunterControlReservation> >
        s_felhunterControlReservations;

    void PruneFelhunterReservations(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return;

        Map* map = ai->GetBot()->GetMap();
        auto itr = s_felhunterControlReservations.find(map);
        if (itr == s_felhunterControlReservations.end())
            return;

        std::vector<FelhunterControlReservation>& reservations = itr->second;

        for (auto r = reservations.begin(); r != reservations.end(); )
        {
            Creature* fel = map->GetCreature(r->felGuid);

            // Reservation ends when the Fel disappears/dies or when core charm
            // ownership has been established. A controlled Warlock is still
            // excluded later by HasAnyControlledFelhunter().
            if (!fel || !fel->IsAlive() || fel->HasCharmer())
                r = reservations.erase(r);
            else
                ++r;
        }

        if (reservations.empty())
            s_felhunterControlReservations.erase(map);
    }

    bool WarlockHasPendingFelhunter(PlayerbotAI* ai, uint32 warlockLowGuid)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return false;

        PruneFelhunterReservations(ai);

        auto itr = s_felhunterControlReservations.find(ai->GetBot()->GetMap());
        if (itr == s_felhunterControlReservations.end())
            return false;

        for (FelhunterControlReservation const& r : itr->second)
        {
            if (r.warlockLowGuid == warlockLowGuid)
                return true;
        }

        return false;
    }

    uint32 ReservedWarlockForFelhunter(PlayerbotAI* ai, Creature* fel)
    {
        if (!ai || !ai->GetBot() || !fel || !ai->GetBot()->GetMap())
            return 0;

        PruneFelhunterReservations(ai);

        auto itr = s_felhunterControlReservations.find(ai->GetBot()->GetMap());
        if (itr == s_felhunterControlReservations.end())
            return 0;

        for (FelhunterControlReservation const& r : itr->second)
        {
            if (r.felGuid == fel->GetObjectGuid())
                return r.warlockLowGuid;
        }

        return 0;
    }

    void ReserveFelhunterForWarlock(
        PlayerbotAI* ai,
        Creature* fel,
        uint32 warlockLowGuid)
    {
        if (!ai || !ai->GetBot() || !fel ||
            !warlockLowGuid || !ai->GetBot()->GetMap())
        {
            return;
        }

        Map* map = ai->GetBot()->GetMap();
        PruneFelhunterReservations(ai);

        std::vector<FelhunterControlReservation>& reservations =
            s_felhunterControlReservations[map];

        // Do not duplicate the same Fel reservation.
        for (FelhunterControlReservation const& r : reservations)
        {
            if (r.felGuid == fel->GetObjectGuid())
                return;
        }

        reservations.push_back(
            FelhunterControlReservation(
                fel->GetObjectGuid(),
                warlockLowGuid));

        sLog.outDetail(
            "[EncounterAI][Maulgar][Felhunter] reserved fel=%s for warlockGuid=%u",
            fel->GetObjectGuid().GetString().c_str(),
            warlockLowGuid);
    }

    void ResetFelhunterReservations(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return;

        s_felhunterControlReservations.erase(ai->GetBot()->GetMap());
    }

    uint8 BlindeyeInterruptRoundForGuid(uint32 guid)
    {
        switch (guid)
        {
            // Round 1 — Warrior x3 per progression raid
            // Raid1: Armsone, Furyone, Protwarone
            case 17: case 22: case 145:
            // Raid2: Armstwo, Furytwo, Protwartwo
            case 39: case 56: case 100:
            // Raid3: Armsthree, Furythree, Protwarthree
            case 83: case 12: case 124:
                return 1;

            // Round 2 — Combat Rogue + Enhancement x2
            // Raid1
            case 32: case 30: case 175:
            // Raid2
            case 53: case 46: case 387:
            // Raid3
            case 126: case 174: case 400:
                return 2;

            // Round 3 — Mage x2 + Elemental Shaman
            // Raid1
            case 20: case 35: case 77:
            // Raid2
            case 40: case 43: case 349:
            // Raid3
            case 114: case 72: case 412:
                return 3;

            default:
                return 0;
        }
    }

    const char* BlindeyeInterruptRoundName(uint8 round)
    {
        switch (round)
        {
            case 1: return "WARRIOR_CHAIN";
            case 2: return "ROGUE_ENHANCEMENT_CHAIN";
            case 3: return "MAGE_ELEMENTAL_CHAIN";
            default: return "NONE";
        }
    }

    bool IsBlindeyeDangerousSpell(Spell* spell)
    {
        if (!spell || !spell->m_spellInfo)
            return false;

        const uint32 spellId = spell->m_spellInfo->Id;
        return spellId == EncounterConstants::SPELL_BLINDEYE_HEAL ||
               spellId == EncounterConstants::SPELL_BLINDEYE_PRAYER;
    }

    uint8 GetBlindeyeInterruptRound(PlayerbotAI* ai, Creature* blindeye)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return 0;

        Map* map = ai->GetBot()->GetMap();
        BlindeyeInterruptState& state = s_blindeyeInterruptState[map];

        Spell* spell =
            blindeye ? blindeye->GetCurrentSpell(CURRENT_GENERIC_SPELL) : nullptr;

        // An explicit no-cast observation separates consecutive casts even if
        // the allocator later reuses the same Spell pointer address.
        if (!IsBlindeyeDangerousSpell(spell))
        {
            state.activeSpell = nullptr;
            state.activeRound = 0;
            state.interrupted = false;
            return 0;
        }

        if (state.activeSpell != spell)
        {
            state.activeSpell = spell;
            state.activeRound = state.nextRound;
            state.nextRound = (state.nextRound % 3) + 1;
            state.interrupted = false;

            sLog.outDetail(
                "[EncounterAI][Maulgar][Blindeye] spell=%u assigned round=%u group=%s",
                spell->m_spellInfo->Id,
                state.activeRound,
                BlindeyeInterruptRoundName(state.activeRound));
        }

        if (state.interrupted)
            return 0;

        return state.activeRound;
    }

    void MarkBlindeyeInterruptSuccess(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return;

        Map* map = ai->GetBot()->GetMap();
        auto itr = s_blindeyeInterruptState.find(map);
        if (itr != s_blindeyeInterruptState.end())
            itr->second.interrupted = true;
    }

    void ResetBlindeyeInterruptChain(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return;

        s_blindeyeInterruptState.erase(ai->GetBot()->GetMap());
    }

    EncounterActor FindProtectedHumanWarlock(PlayerbotAI* ai)
    {
        for (uint32 guid : HUMAN_OLM_WARLOCKS)
        {
            EncounterActor actor = EncounterActorResolver::Find(ai, guid);
            if (actor.IsValid() && actor.IsHuman())
                return actor;
        }
        return EncounterActor();
    }

    bool HumanWarlockIsFreeForNewFelhunter(
        PlayerbotAI* ai,
        EncounterActor const& humanWarlock)
    {
        if (!humanWarlock.IsValid() || !humanWarlock.IsHuman())
            return false;

        // A human already owning a controlled Fel is considered occupied.
        // We do not reserve pending casts for the human because server AI does
        // not issue the cast; human intent is not knowable. Once the human has
        // one controlled Fel, later summons are free to go to RNDBOT Warlocks.
        return !HighKingMaulgarEncounter::HasAnyControlledFelhunter(
            ai,
            humanWarlock.player);
    }

    EncounterActor FirstPresentBotWarlock(PlayerbotAI* ai)
    {
        for (uint32 guid : BOT_OLM_WARLOCKS)
        {
            EncounterActor actor = EncounterActorResolver::Find(ai, guid);
            if (actor.IsValid() && actor.IsBot())
                return actor;
        }
        return EncounterActor();
    }

    // Select a bot Warlock that does not already own an enslaved Wild Fel
    // Stalker. This lets successive summons be distributed across the multiple
    // Destruction warlocks in a 25-man progression raid instead of repeatedly
    // replacing one bot's existing enslaved demon.
    EncounterActor FirstFreeBotWarlock(PlayerbotAI* ai)
    {
        for (uint32 guid : BOT_OLM_WARLOCKS)
        {
            EncounterActor actor = EncounterActorResolver::Find(ai, guid);
            if (!actor.IsValid() || !actor.IsBot())
                continue;

            // "Free" means neither already controlling a Fel Stalker nor
            // reserved for another uncontrolled Fel whose Enslave is pending.
            if (!HighKingMaulgarEncounter::HasAnyControlledFelhunter(ai, actor.player) &&
                !WarlockHasPendingFelhunter(ai, guid))
            {
                return actor;
            }
        }
        return EncounterActor();
    }

    bool CurrentBotControlsFelhunter(PlayerbotAI* ai)
    {
        return ai && ai->GetBot() &&
               HighKingMaulgarEncounter::HasAnyControlledFelhunter(ai, ai->GetBot());
    }
}

Creature* HighKingMaulgarEncounter::FindCreature(PlayerbotAI* ai, uint32 entry)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    return GetClosestCreatureWithEntry(
        ai->GetBot(), entry, MAULGAR_SEARCH_RANGE, true);
}

Creature* HighKingMaulgarEncounter::FindUncontrolledFelhunter(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    CreatureList list;
    GetCreatureListWithEntryInGrid(
        list,
        ai->GetBot(),
        EncounterConstants::NPC_WILD_FEL_STALKER,
        MAULGAR_SEARCH_RANGE);

    Creature* nearest = nullptr;
    float nearestDistance = MAULGAR_SEARCH_RANGE + 1.0f;

    for (Creature* creature : list)
    {
        if (!creature || !creature->IsAlive())
            continue;

        // Exact core ownership state. A charmed Fel Stalker is no longer a
        // Paladin pickup target and must never enter the raid kill order.
        if (creature->HasCharmer())
            continue;

        const float distance = ai->GetBot()->GetDistance(creature);
        if (!nearest || distance < nearestDistance)
        {
            nearest = creature;
            nearestDistance = distance;
        }
    }

    return nearest;
}

Creature* HighKingMaulgarEncounter::FindAnyLivingFelhunter(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    CreatureList list;
    GetCreatureListWithEntryInGrid(
        list,
        ai->GetBot(),
        EncounterConstants::NPC_WILD_FEL_STALKER,
        MAULGAR_SEARCH_RANGE);

    Creature* nearest = nullptr;
    float nearestDistance = MAULGAR_SEARCH_RANGE + 1.0f;

    for (Creature* creature : list)
    {
        if (!creature || !creature->IsAlive())
            continue;

        const float distance = ai->GetBot()->GetDistance(creature);
        if (!nearest || distance < nearestDistance)
        {
            nearest = creature;
            nearestDistance = distance;
        }
    }

    return nearest;
}

uint32 HighKingMaulgarEncounter::CountLivingFelhunter(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return 0;

    CreatureList list;
    GetCreatureListWithEntryInGrid(
        list,
        ai->GetBot(),
        EncounterConstants::NPC_WILD_FEL_STALKER,
        MAULGAR_SEARCH_RANGE);

    uint32 alive = 0;
    for (Creature* creature : list)
    {
        if (creature && creature->IsAlive())
            ++alive;
    }

    return alive;
}

Creature* HighKingMaulgarEncounter::FindFelhunterControlledBy(
    PlayerbotAI* ai, Unit* charmer)
{
    if (!ai || !ai->GetBot() || !charmer)
        return nullptr;

    CreatureList list;
    GetCreatureListWithEntryInGrid(
        list,
        ai->GetBot(),
        EncounterConstants::NPC_WILD_FEL_STALKER,
        MAULGAR_SEARCH_RANGE);

    for (Creature* creature : list)
    {
        if (!creature || !creature->IsAlive())
            continue;

        if (creature->HasCharmer(charmer->GetObjectGuid()))
            return creature;
    }

    return nullptr;
}

bool HighKingMaulgarEncounter::HasAnyControlledFelhunter(
    PlayerbotAI* ai, Unit* charmer)
{
    return FindFelhunterControlledBy(ai, charmer) != nullptr;
}

uint32 HighKingMaulgarEncounter::HighestEnslaveDemonSpell(PlayerbotAI* ai)
{
    if (!ai)
        return 0;

    if (ai->HasSpell(EncounterConstants::SPELL_ENSLAVE_DEMON_R3))
        return EncounterConstants::SPELL_ENSLAVE_DEMON_R3;
    if (ai->HasSpell(EncounterConstants::SPELL_ENSLAVE_DEMON_R2))
        return EncounterConstants::SPELL_ENSLAVE_DEMON_R2;
    if (ai->HasSpell(EncounterConstants::SPELL_ENSLAVE_DEMON_R1))
        return EncounterConstants::SPELL_ENSLAVE_DEMON_R1;

    return 0;
}

void HighKingMaulgarEncounter::DriveControlledFelhunter(
    PlayerbotAI* ai,
    Unit* charmer,
    Creature* krosh,
    Creature* maulgar)
{
    if (!ai || !charmer)
        return;

    Creature* fel = FindFelhunterControlledBy(ai, charmer);
    if (!fel || !fel->IsAlive())
        return;

    Unit* sacrificeTarget = nullptr;

    // User-defined encounter policy:
    // 1. immediately after Enslave, Wild Fel Stalker melees Krosh;
    // 2. after Krosh dies, it switches directly to Maulgar;
    // 3. while Maulgar Whirlwinds it does NOT flee -- staying in melee is
    //    intentional so the add is preferentially consumed by boss AoE.
    if (krosh && krosh->IsAlive())
        sacrificeTarget = krosh;
    else if (maulgar && maulgar->IsAlive())
        sacrificeTarget = maulgar;

    if (!sacrificeTarget)
        return;

    // Reassert melee engagement every encounter update. Charmed creatures can
    // otherwise drift back to their charmer after control transitions.
    fel->Attack(sacrificeTarget, true);
    fel->GetMotionMaster()->MoveChase(
        sacrificeTarget,
        0.0f,
        0.0f,
        false,
        false,
        true,
        false);
}

uint32 HighKingMaulgarEncounter::ReleaseAllControlledFelhunters(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return 0;

    CreatureList list;
    GetCreatureListWithEntryInGrid(
        list,
        ai->GetBot(),
        EncounterConstants::NPC_WILD_FEL_STALKER,
        MAULGAR_SEARCH_RANGE);

    uint32 released = 0;

    for (Creature* fel : list)
    {
        if (!fel || !fel->IsAlive() || !fel->HasCharmer())
            continue;

        Unit* charmer = fel->GetCharmer(ai->GetBot());
        if (!charmer)
            continue;

        // Core API physically reverts the charm relationship. This is used only
        // after Maulgar is DONE so surviving sacrifice pets become hostile
        // cleanup targets for the raid.
        charmer->Uncharm(fel);
        ++released;
    }

    return released;
}

EncounterOverrideResult HighKingMaulgarEncounter::HandlePostKillFelhunterCleanup(
    PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return EncounterOverrideResult::NotHandled;

    // First pass: release every surviving enslaved Fel Stalker.
    const uint32 released = ReleaseAllControlledFelhunters(ai);
    if (released > 0)
    {
        sLog.outDetail(
            "[EncounterAI][Maulgar] released %u surviving Wild Fel Stalker(s) for post-kill cleanup",
            released);
    }

    Creature* cleanupTarget = FindAnyLivingFelhunter(ai);
    if (!cleanupTarget)
        return EncounterOverrideResult::NotHandled;

    const uint32 botGuid = ai->GetBot()->GetObjectGuid().GetCounter();

    // Healers keep their normal healing engine. Everyone else focuses the same
    // nearest surviving Fel Stalker until all are dead.
    if (IsRaidHealer(botGuid))
        return EncounterOverrideResult::NotHandled;

    SetEncounterTarget(ai, cleanupTarget);

    // Soft override: use existing Normal Rotation to kill the hostile add.
    return EncounterOverrideResult::NotHandled;
}

void HighKingMaulgarEncounter::SetEncounterTarget(
    PlayerbotAI* ai, Unit* target)
{
    if (!ai || !target || !target->IsAlive())
        return;

    AiObjectContext* context = ai->GetAiObjectContext();
    if (!context)
        return;

    context->GetValue<Unit*>("current target")->Set(target);
    context->GetValue<ObjectGuid>("attack target")->Set(target->GetObjectGuid());
}

Unit* HighKingMaulgarEncounter::SelectKillOrderTarget(
    Creature* blindeye,
    Creature* olm,
    Creature* kiggler,
    Creature* krosh,
    Creature* maulgar)
{
    // Wild Fel Stalkers are intentionally absent from this list.
    if (blindeye && blindeye->IsAlive()) return blindeye;
    if (olm      && olm->IsAlive())      return olm;
    if (kiggler  && kiggler->IsAlive())  return kiggler;
    if (krosh    && krosh->IsAlive())    return krosh;
    if (maulgar  && maulgar->IsAlive())  return maulgar;
    return nullptr;
}

bool HighKingMaulgarEncounter::IsRaidHealer(uint32 guid)
{
    switch (guid)
    {
        case 41: case 61: case 122: case 372: case 10:
        case 69: case 2: case 147: case 422: case 51:
        case 106: case 120: case 355: case 526: case 104:
            return true;
        default:
            return false;
    }
}

bool HighKingMaulgarEncounter::IsMeleeDps(uint32 guid)
{
    switch (guid)
    {
        case 17: case 22: case 37: case 32: case 30: case 175:
        case 39: case 56: case 63: case 53: case 46: case 387:
        case 83: case 12: case 103: case 126: case 174: case 400:
            return true;
        default:
            return false;
    }
}

bool HighKingMaulgarEncounter::IsBlindeyeHealing(Creature* blindeye)
{
    if (!blindeye)
        return false;

    Spell* spell = blindeye->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell || !spell->m_spellInfo)
        return false;

    const uint32 spellId = spell->m_spellInfo->Id;
    return spellId == EncounterConstants::SPELL_BLINDEYE_HEAL ||
           spellId == EncounterConstants::SPELL_BLINDEYE_PRAYER;
}

EncounterOverrideResult HighKingMaulgarEncounter::HandleKroshMage(
    PlayerbotAI* ai, Creature* krosh)
{
    if (!ai || !krosh || !krosh->IsAlive())
        return EncounterOverrideResult::NotHandled;

    SetEncounterTarget(ai, krosh);

    if (ai->HasAura(EncounterConstants::SPELL_KROSH_SPELL_SHIELD, krosh) &&
        !ai->HasAura(EncounterConstants::SPELL_KROSH_SPELL_SHIELD, ai->GetBot()) &&
        ai->HasSpell(EncounterConstants::SPELL_SPELLSTEAL))
    {
        if (ai->CastSpell(EncounterConstants::SPELL_SPELLSTEAL, krosh))
        {
            sLog.outDetail(
                "[EncounterAI][Maulgar] %s stole Krosh Spell Shield",
                ai->GetBot()->GetName());
            return EncounterOverrideResult::Handled;
        }
    }

    if (MaulgarFormationManager::EnsureKroshMagePosition(ai, krosh))
        return EncounterOverrideResult::BlockNormal;

    return EncounterOverrideResult::NotHandled;
}

EncounterOverrideResult HighKingMaulgarEncounter::HandleOlmWarlock(
    PlayerbotAI* ai,
    Creature* olm,
    Creature* uncontrolledFelhunter)
{
    if (!ai || !ai->GetBot())
        return EncounterOverrideResult::NotHandled;

    // Priority 1: capture a freshly summoned Wild Fel Stalker.
    if (uncontrolledFelhunter && uncontrolledFelhunter->IsAlive())
    {
        const uint32 enslaveSpell = HighestEnslaveDemonSpell(ai);

        // Never attack the demon while we are trying to capture it.
        SetEncounterTarget(ai, uncontrolledFelhunter);

        if (!enslaveSpell)
        {
            // No substitute CC path exists. Leave the Fel Stalker on the
            // Paladin and return this Warlock to Olm instead of damaging it.
            sLog.outError(
                "[EncounterAI][Maulgar] %s cannot control Wild Fel Stalker: "
                "Enslave Demon is not learned; Paladin remains pickup tank",
                ai->GetBot()->GetName());

            if (olm && olm->IsAlive())
                SetEncounterTarget(ai, olm);

            return EncounterOverrideResult::NotHandled;
        }

        if (ai->CastSpell(enslaveSpell, uncontrolledFelhunter))
        {
            sLog.outDetail(
                "[EncounterAI][Maulgar] %s started Enslave Demon(%u) on Wild Fel Stalker",
                ai->GetBot()->GetName(), enslaveSpell);

            // The 3-second cast is a HARD encounter action. Paladin continues
            // tanking until the core sets UNIT_FIELD_CHARMEDBY.
            return EncounterOverrideResult::Handled;
        }

        // Do not let normal Warlock DPS accidentally kill the intended control
        // target on a failed/resisted/out-of-range attempt. The next AI tick
        // retries Enslave Demon. The Paladin remains on the demon meanwhile.
        return EncounterOverrideResult::BlockNormal;
    }

    // Priority 2: once control is established, immediately restore Olm as the
    // Warlock's combat target. Paladin release is driven independently by
    // creature->HasCharmer().
    if (olm && olm->IsAlive())
    {
        SetEncounterTarget(ai, olm);

        // Threat spell remains available while already positioned. During a
        // larger correction, encounter movement owns the tick.
        if (MaulgarFormationManager::EnsureOlmWarlockPosition(ai, olm))
            return EncounterOverrideResult::BlockNormal;

        if (ai->HasSpell("searing pain") && ai->CastSpell("searing pain", olm))
            return EncounterOverrideResult::Handled;
    }

    return EncounterOverrideResult::NotHandled;
}

EncounterOverrideResult HighKingMaulgarEncounter::HandleBlindeyeInterrupt(
    PlayerbotAI* ai, Creature* blindeye)
{
    if (!ai || !ai->GetBot() || !blindeye || !blindeye->IsAlive())
        return EncounterOverrideResult::NotHandled;

    Spell* spell = blindeye->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!IsBlindeyeDangerousSpell(spell))
    {
        // Also clears the active-cast marker between casts.
        GetBlindeyeInterruptRound(ai, blindeye);
        return EncounterOverrideResult::NotHandled;
    }

    const uint8 activeRound = GetBlindeyeInterruptRound(ai, blindeye);
    const uint32 botGuid = ai->GetBot()->GetObjectGuid().GetCounter();
    const uint8 botRound = BlindeyeInterruptRoundForGuid(botGuid);

    // Strict chain: bots outside the assigned round do not spend interrupts.
    if (!activeRound || botRound != activeRound)
        return EncounterOverrideResult::NotHandled;

    SetEncounterTarget(ai, blindeye);

    bool success = false;
    const char* ability = "NONE";

    switch (activeRound)
    {
        // ---------------------------------------------------------------
        // ROUND 1:
        //   Arms + Fury + Protection Warrior
        //   Shield Bash first; Pummel is the same-round fallback.
        // ---------------------------------------------------------------
        case 1:
            if (ai->GetBot()->getClass() != CLASS_WARRIOR)
                return EncounterOverrideResult::NotHandled;

            if (ai->HasSpell("shield bash"))
            {
                ability = "Shield Bash";
                success = ai->CastSpell("shield bash", blindeye);
            }

            if (!success && ai->HasSpell("pummel"))
            {
                ability = "Pummel";
                success = ai->CastSpell("pummel", blindeye);
            }
            break;

        // ---------------------------------------------------------------
        // ROUND 2:
        //   Combat Rogue -> Kick
        //   Enhancement Shaman x2 -> Earth Shock
        // ---------------------------------------------------------------
        case 2:
            if (ai->GetBot()->getClass() == CLASS_ROGUE &&
                ai->HasSpell("kick"))
            {
                ability = "Kick";
                success = ai->CastSpell("kick", blindeye);
            }
            else if (ai->GetBot()->getClass() == CLASS_SHAMAN &&
                     ai->HasSpell("earth shock"))
            {
                ability = "Earth Shock";
                success = ai->CastSpell("earth shock", blindeye);
            }
            break;

        // ---------------------------------------------------------------
        // ROUND 3:
        //   Arcane/Fire Mage -> Counterspell
        //   Elemental Shaman -> Earth Shock
        //
        // The Mage currently assigned to Krosh is allowed to perform this
        // instant hard override on Blindeye, then returns to Krosh next tick.
        // ---------------------------------------------------------------
        case 3:
            if (ai->GetBot()->getClass() == CLASS_MAGE &&
                ai->HasSpell("counterspell"))
            {
                ability = "Counterspell";
                success = ai->CastSpell("counterspell", blindeye);
            }
            else if (ai->GetBot()->getClass() == CLASS_SHAMAN &&
                     ai->HasSpell("earth shock"))
            {
                ability = "Earth Shock";
                success = ai->CastSpell("earth shock", blindeye);
            }
            break;

        default:
            return EncounterOverrideResult::NotHandled;
    }

    if (success)
    {
        MarkBlindeyeInterruptSuccess(ai);

        sLog.outDetail(
            "[EncounterAI][Maulgar][Blindeye] round=%u group=%s bot=%s guid=%u used=%s spell=%u SUCCESS",
            activeRound,
            BlindeyeInterruptRoundName(activeRound),
            ai->GetBot()->GetName(),
            botGuid,
            ability,
            spell && spell->m_spellInfo ? spell->m_spellInfo->Id : 0);

        return EncounterOverrideResult::Handled;
    }

    // Same-round redundancy is deliberate: if this member cannot execute due
    // to stance/range/GCD/cooldown, another member of the same round may still
    // get its Update before Blindeye finishes the cast.
    return EncounterOverrideResult::NotHandled;
}

EncounterOverrideResult HighKingMaulgarEncounter::Update(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return EncounterOverrideResult::NotHandled;

    Player* bot = ai->GetBot();

    if (bot->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
        return EncounterOverrideResult::NotHandled;

    InstanceData* instance =
        bot->GetMap() ? bot->GetMap()->GetInstanceData() : nullptr;

    if (!instance)
        return EncounterOverrideResult::NotHandled;

    const uint32 encounterState =
        instance->GetData(EncounterConstants::TYPE_MAULGAR_EVENT);

    EncounterTrace::EncounterState(ai, "MAULGAR", encounterState);

    // NOT_STARTED is now an active pre-pull state: exact fixed anchors,
    // Misdirection arming, and Mage/Hunter synchronized pull barrier.
    if (encounterState == 0) // NOT_STARTED
    {
        ResetBlindeyeInterruptChain(ai);
        ResetFelhunterReservations(ai);
        MaulgarFormationManager::Reset(ai);

        return MaulgarPullCoordinator::UpdatePrePull(ai);
    }

    // Maulgar DONE is not an immediate overlay exit if enslaved/surviving
    // Wild Fel Stalkers remain. Release and clear them first.
    if (encounterState == EncounterConstants::ENCOUNTER_DONE)
    {
        ResetBlindeyeInterruptChain(ai);
        ResetFelhunterReservations(ai);
        MaulgarFormationManager::Reset(ai);
        MaulgarPullCoordinator::Reset(ai);

        if (CountLivingFelhunter(ai) > 0)
            return HandlePostKillFelhunterCleanup(ai);

        return EncounterOverrideResult::NotHandled;
    }

    if (encounterState != EncounterConstants::ENCOUNTER_IN_PROGRESS)
    {
        // FAIL/SPECIAL clears transient pull/interrupt state.
        ResetBlindeyeInterruptChain(ai);
        ResetFelhunterReservations(ai);
        MaulgarFormationManager::Reset(ai);
        MaulgarPullCoordinator::Reset(ai);
        return EncounterOverrideResult::NotHandled;
    }

    // First combat ticks: each Hunter remains locked to its assigned council
    // member until the core Misdirection redirection target clears after the
    // opener. Other roles continue into normal encounter logic immediately.
    EncounterOverrideResult opening =
        MaulgarPullCoordinator::UpdateOpening(ai);

    if (opening != EncounterOverrideResult::NotHandled)
        return opening;

    Creature* maulgar  = FindCreature(ai, EncounterConstants::NPC_MAULGAR);
    Creature* krosh    = FindCreature(ai, EncounterConstants::NPC_KROSH);
    Creature* olm      = FindCreature(ai, EncounterConstants::NPC_OLM);
    Creature* kiggler  = FindCreature(ai, EncounterConstants::NPC_KIGGLER);
    Creature* blindeye = FindCreature(ai, EncounterConstants::NPC_BLINDEYE);

    MaulgarFormationManager::EnsureMaulgarFrame(
        ai, maulgar, krosh, olm, kiggler, blindeye);

    // Exact Felhunter state derived from UNIT_FIELD_CHARMEDBY.
    Creature* uncontrolledFelhunter = FindUncontrolledFelhunter(ai);

    const uint32 botGuid = bot->GetObjectGuid().GetCounter();

    EncounterActor maulgarTank = EncounterActorResolver::FirstAvailable(
        ai, { MAULGAR_FERAL_TANKS[0], MAULGAR_FERAL_TANKS[1], MAULGAR_FERAL_TANKS[2] });

    // Blindeye is now explicitly Warrior-first. Prot Paladin is reserved for
    // freshly summoned Wild Fel Stalkers.
    EncounterActor blindeyeWarrior = EncounterActorResolver::FirstAvailable(
        ai, { BLINDEYE_WARRIOR_TANKS[0], BLINDEYE_WARRIOR_TANKS[1], BLINDEYE_WARRIOR_TANKS[2] });

    EncounterActor felhunterPaladin = EncounterActorResolver::FirstAvailable(
        ai, { FELHUNTER_PALADIN_TANKS[0], FELHUNTER_PALADIN_TANKS[1], FELHUNTER_PALADIN_TANKS[2] });

    EncounterActor kigglerTank = EncounterActorResolver::FirstAvailable(
        ai, { KIGGLER_BALANCE[0], KIGGLER_BALANCE[1], KIGGLER_BALANCE[2] });

    EncounterActor kroshController =
        EncounterActorResolver::PreferredHumanOrFallback(
            ai,
            { HUMAN_MAGE_TANKS[0], HUMAN_MAGE_TANKS[1] },
            { BOT_MAGE_TANKS[0], BOT_MAGE_TANKS[1], BOT_MAGE_TANKS[2],
              BOT_MAGE_TANKS[3], BOT_MAGE_TANKS[4], BOT_MAGE_TANKS[5] });

    EncounterActor humanOlmWarlock = FindProtectedHumanWarlock(ai);
    EncounterActor primaryBotWarlock = FirstPresentBotWarlock(ai);

    EncounterTrace::Assignment(ai, "MAULGAR", "MAULGAR_MT", maulgarTank);
    EncounterTrace::Assignment(ai, "MAULGAR", "BLINDEYE_TANK", blindeyeWarrior);
    EncounterTrace::Assignment(ai, "MAULGAR", "FELHUNTER_PALADIN", felhunterPaladin);
    EncounterTrace::Assignment(ai, "MAULGAR", "KIGGLER_TANK", kigglerTank);
    EncounterTrace::Assignment(ai, "MAULGAR", "KROSH_CONTROLLER", kroshController);
    EncounterTrace::Assignment(ai, "MAULGAR", "OLM_HUMAN_WARLOCK", humanOlmWarlock);
    EncounterTrace::Assignment(ai, "MAULGAR", "OLM_BOT_WARLOCK", primaryBotWarlock);

    EncounterTrace::ProtectedHuman(ai, "MAULGAR", "KROSH_CONTROLLER", kroshController);
    EncounterTrace::ProtectedHuman(ai, "MAULGAR", "OLM_WARLOCK", humanOlmWarlock);

    // If the protected human Warlock has already enslaved a Fel Stalker, the
    // Encounter layer may drive the enslaved creature itself to Krosh/Maulgar.
    // It still never casts or moves the human player.
    if (humanOlmWarlock.IsValid() && humanOlmWarlock.IsHuman())
        DriveControlledFelhunter(ai, humanOlmWarlock.player, krosh, maulgar);

    // ---------------------------------------------------------------------
    // Blindeye strict 3-round interrupt chain has encounter-hard priority.
    // Only members of the currently assigned round are allowed to act.
    // ---------------------------------------------------------------------
    EncounterOverrideResult blindeyeInterrupt =
        HandleBlindeyeInterrupt(ai, blindeye);

    if (blindeyeInterrupt != EncounterOverrideResult::NotHandled)
        return blindeyeInterrupt;

    // ---------------------------------------------------------------------
    // 1. Freshly summoned Fel Stalker: Prot Paladin pickup has first priority.
    // ---------------------------------------------------------------------
    if (uncontrolledFelhunter &&
        EncounterActorResolver::IsCurrentBot(ai, felhunterPaladin))
    {
        SetEncounterTarget(ai, uncontrolledFelhunter);

        // Normal Protection Paladin tank rotation handles initial pickup and
        // threat. As soon as HasCharmer() becomes true, this branch disappears
        // on the next AI tick and the Paladin is released.
        return EncounterOverrideResult::NotHandled;
    }

    // ---------------------------------------------------------------------
    // 2. Warlock control handoff.
    // ---------------------------------------------------------------------
    if (uncontrolledFelhunter)
    {
        // Pickup and Enslave are PARALLEL responsibilities:
        //
        // - on the Prot Paladin's Update, it immediately tanks the Fel;
        // - on the assigned Warlock's Update, it immediately starts Enslave.
        //
        // There is deliberately no "wait for Paladin threat" handoff timer.
        const bool humanFree =
            HumanWarlockIsFreeForNewFelhunter(ai, humanOlmWarlock);

        // A free protected human Chuchote gets priority for the first Fel, but
        // only while not already controlling one. Once occupied, later summons
        // are allowed to use RNDBOT Warlocks.
        if (!humanFree)
        {
            uint32 reservedGuid =
                ReservedWarlockForFelhunter(ai, uncontrolledFelhunter);

            EncounterActor assignedWarlock;

            if (reservedGuid)
            {
                assignedWarlock =
                    EncounterActorResolver::Find(ai, reservedGuid);
            }
            else
            {
                assignedWarlock = FirstFreeBotWarlock(ai);

                if (assignedWarlock.IsValid() && assignedWarlock.IsBot())
                {
                    ReserveFelhunterForWarlock(
                        ai,
                        uncontrolledFelhunter,
                        assignedWarlock.LowGuid());
                }
            }

            if (EncounterActorResolver::IsCurrentBot(ai, assignedWarlock))
            {
                return HandleOlmWarlock(
                    ai,
                    olm,
                    uncontrolledFelhunter);
            }
        }
    }

    // Any automated Warlock that controls a Fel Stalker uses it as a disposable
    // melee unit: Krosh first, then Maulgar. The Warlock itself returns to Olm.
    if (CurrentBotControlsFelhunter(ai))
    {
        DriveControlledFelhunter(ai, bot, krosh, maulgar);
        return HandleOlmWarlock(ai, olm, nullptr);
    }

    // Primary automated Olm Warlock before/without a Fel Stalker.
    if (!HumanWarlockIsFreeForNewFelhunter(ai, humanOlmWarlock) &&
        EncounterActorResolver::IsCurrentBot(ai, primaryBotWarlock))
    {
        return HandleOlmWarlock(ai, olm, nullptr);
    }

    // ---------------------------------------------------------------------
    // Other dedicated assignments.
    // ---------------------------------------------------------------------
    if (EncounterActorResolver::IsCurrentBot(ai, maulgarTank) &&
        maulgar && maulgar->IsAlive())
    {
        SetEncounterTarget(ai, maulgar);

        if (MaulgarFormationManager::EnsureTankAnchor(
                ai, maulgar, MaulgarTankAnchorRole::Maulgar))
        {
            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::NotHandled;
    }

    if (EncounterActorResolver::IsCurrentBot(ai, blindeyeWarrior) &&
        blindeye && blindeye->IsAlive())
    {
        SetEncounterTarget(ai, blindeye);

        if (MaulgarFormationManager::EnsureTankAnchor(
                ai, blindeye, MaulgarTankAnchorRole::Blindeye))
        {
            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::NotHandled;
    }

    // If no Protection Warrior exists, Prot Paladin may cover Blindeye only
    // while there is no hostile/uncontrolled Fel Stalker requiring pickup.
    if (!blindeyeWarrior.IsValid() &&
        !uncontrolledFelhunter &&
        EncounterActorResolver::IsCurrentBot(ai, felhunterPaladin) &&
        blindeye && blindeye->IsAlive())
    {
        SetEncounterTarget(ai, blindeye);

        if (MaulgarFormationManager::EnsureTankAnchor(
                ai, blindeye, MaulgarTankAnchorRole::Blindeye))
        {
            return EncounterOverrideResult::BlockNormal;
        }

        return EncounterOverrideResult::NotHandled;
    }

    if (EncounterActorResolver::IsCurrentBot(ai, kigglerTank) &&
        kiggler && kiggler->IsAlive())
    {
        SetEncounterTarget(ai, kiggler);

        if (MaulgarFormationManager::EnsureKigglerPosition(ai, kiggler))
            return EncounterOverrideResult::BlockNormal;

        return EncounterOverrideResult::NotHandled;
    }

    if (EncounterActorResolver::IsCurrentBot(ai, kroshController))
        return HandleKroshMage(ai, krosh);

    // When no hostile Fel requires pickup, keep the Protection Paladin in the
    // Olm/Blindeye half of the room instead of letting generic combat movement
    // drift it toward Krosh/Maulgar.
    if (!uncontrolledFelhunter &&
        EncounterActorResolver::IsCurrentBot(ai, felhunterPaladin))
    {
        if (MaulgarFormationManager::EnsureTankAnchor(
                ai, nullptr, MaulgarTankAnchorRole::FelhunterStandby))
        {
            return EncounterOverrideResult::BlockNormal;
        }
    }

    if (IsRaidHealer(botGuid))
    {
        if (MaulgarFormationManager::EnsureHealerPosition(ai))
            return EncounterOverrideResult::BlockNormal;

        return EncounterOverrideResult::NotHandled;
    }

    Unit* killTarget =
        SelectKillOrderTarget(blindeye, olm, kiggler, krosh, maulgar);

    if (!killTarget)
        return EncounterOverrideResult::NotHandled;

    SetEncounterTarget(ai, killTarget);

    if (killTarget == maulgar &&
        maulgar &&
        ai->HasAura(EncounterConstants::SPELL_MAULGAR_WHIRLWIND, maulgar) &&
        IsMeleeDps(botGuid))
    {
        // Deterministic radial evacuation replaces generic random flee when the
        // formation frame is available. Controlled Fel Stalkers are not player
        // bots and therefore remain on Maulgar as intended.
        if (MaulgarFormationManager::HandleMaulgarWhirlwind(ai, maulgar))
            return EncounterOverrideResult::BlockNormal;

        // Compatibility fallback if the formation frame was unavailable.
        if (ai->DoSpecificAction("flee", Event(), true))
            return EncounterOverrideResult::Handled;

        return EncounterOverrideResult::BlockNormal;
    }

    if (MaulgarFormationManager::IsMeleeFormationActor(botGuid))
    {
        if (MaulgarFormationManager::EnsureMeleePosition(ai, killTarget))
            return EncounterOverrideResult::BlockNormal;
    }
    else
    {
        if (MaulgarFormationManager::EnsureRangedPosition(ai, killTarget))
            return EncounterOverrideResult::BlockNormal;
    }

    return EncounterOverrideResult::NotHandled;
}
