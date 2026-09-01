#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulEncounter.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.h"
#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/PlayerbotAI.h"

#include "AI/ScriptDevAI/include/sc_grid_searchers.h"
#include "Maps/InstanceData.h"
#include "playerbot/strategy/AiObjectContext.h"

using namespace ai;

namespace
{
    constexpr float GRUUL_SEARCH_RANGE = 220.0f;

    // Progression roster priorities. One member from each list resolves in the
    // active raid; the ordering matches the existing Maulgar assignment policy.
    const uint32 GRUUL_MAIN_TANKS[] =
    {
        15, 24, 88 // Feral tanks: Raid1, Raid2, Raid3
    };

    const uint32 GRUUL_HURTFUL_SOAKERS[] =
    {
        145, 100, 124, // Protection Warriors
        31, 97, 98     // Protection Paladin fallbacks
    };
}

Creature* GruulEncounter::FindGruul(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return nullptr;

    return GetClosestCreatureWithEntry(
        ai->GetBot(),
        EncounterConstants::NPC_GRUUL,
        GRUUL_SEARCH_RANGE,
        true);
}

void GruulEncounter::SetEncounterTarget(
    PlayerbotAI* ai,
    Unit* target)
{
    if (!ai || !target || !target->IsAlive())
        return;

    AiObjectContext* context = ai->GetAiObjectContext();
    if (!context)
        return;

    context->GetValue<Unit*>("current target")->Set(target);
    context->GetValue<ObjectGuid>("attack target")->Set(
        target->GetObjectGuid());
}

bool GruulEncounter::IsRaidHealer(uint32 guid)
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

EncounterOverrideResult GruulEncounter::Update(PlayerbotAI* ai)
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
        instance->GetData(EncounterConstants::TYPE_GRUUL_EVENT);

    EncounterTrace::EncounterState(ai, "GRUUL", encounterState);

    if (encounterState != EncounterConstants::ENCOUNTER_IN_PROGRESS)
    {
        GruulShatterPlanner::Reset(ai);
        return EncounterOverrideResult::NotHandled;
    }

    Creature* gruul = FindGruul(ai);
    if (!gruul || !gruul->IsAlive())
    {
        GruulShatterPlanner::Reset(ai);
        return EncounterOverrideResult::NotHandled;
    }

    EncounterTrace::EventOnce(
        ai,
        "GRUUL",
        "fight-active",
        "FIGHT_ACTIVE",
        "npc=%u",
        EncounterConstants::NPC_GRUUL);

    // Ground Slam/Shatter movement is a hard encounter override and takes
    // precedence over every class rotation and threat assignment.
    EncounterOverrideResult shatter =
        GruulShatterPlanner::Update(ai, gruul);

    if (shatter != EncounterOverrideResult::NotHandled)
        return shatter;

    EncounterActor mainTank =
        EncounterActorResolver::FirstAvailable(
            ai,
            {
                GRUUL_MAIN_TANKS[0],
                GRUUL_MAIN_TANKS[1],
                GRUUL_MAIN_TANKS[2]
            });

    EncounterActor hurtfulSoaker =
        EncounterActorResolver::FirstAvailable(
            ai,
            {
                GRUUL_HURTFUL_SOAKERS[0],
                GRUUL_HURTFUL_SOAKERS[1],
                GRUUL_HURTFUL_SOAKERS[2],
                GRUUL_HURTFUL_SOAKERS[3],
                GRUUL_HURTFUL_SOAKERS[4],
                GRUUL_HURTFUL_SOAKERS[5]
            });

    EncounterTrace::Assignment(ai, "GRUUL", "MAIN_TANK", mainTank);
    EncounterTrace::Assignment(
        ai, "GRUUL", "HURTFUL_SOAKER", hurtfulSoaker);

    if (!mainTank.IsValid())
    {
        EncounterTrace::EventOnce(
            ai,
            "GRUUL",
            "missing-main-tank",
            "ASSIGNMENT_MISSING",
            "role=MAIN_TANK");
    }

    if (!hurtfulSoaker.IsValid())
    {
        EncounterTrace::EventOnce(
            ai,
            "GRUUL",
            "missing-hurtful-soaker",
            "ASSIGNMENT_MISSING",
            "role=HURTFUL_SOAKER");
    }

    // Both tanks stay on Gruul so the secondary tank can maintain the highest
    // non-victim melee threat required by Hurtful Strike. Class tank rotations
    // remain responsible for the actual threat abilities.
    if (EncounterActorResolver::IsCurrentBot(ai, mainTank) ||
        EncounterActorResolver::IsCurrentBot(ai, hurtfulSoaker))
    {
        SetEncounterTarget(ai, gruul);
        return EncounterOverrideResult::NotHandled;
    }

    // Preserve healer target selection. All other bots explicitly focus Gruul
    // after a Shatter window releases the hard movement overlay.
    const uint32 botGuid = bot->GetObjectGuid().GetCounter();
    if (!IsRaidHealer(botGuid))
        SetEncounterTarget(ai, gruul);

    return EncounterOverrideResult::NotHandled;
}
