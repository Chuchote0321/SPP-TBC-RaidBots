#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Internal.h"

#include "playerbot/PlayerbotAI.h"

#include <map>

using namespace ai;
using namespace ai::karazhan_phase1_detail;

namespace
{
    constexpr uint8 RAID_ICON_SKULL = 7;

    const char* const MOROES_GUESTS[] =
    {
        "baroness dorothea millstipe",
        "lady catriona von'indi",
        "lady keira berrybuck",
        "baron rafe dreuger",
        "lord robin daris",
        "lord crispin ference"
    };

    std::map<Map*, ObjectGuid> s_markedGuest;

    Unit* FirstGuest(PlayerbotAI* ai)
    {
        for (const char* name : MOROES_GUESTS)
        {
            Unit* guest = KarazhanPhase1Runtime::FindTarget(ai, name);
            if (guest && guest->IsAlive())
                return guest;
        }

        return nullptr;
    }
}

bool KarazhanPhase1Runtime::HasMoroesGuest(PlayerbotAI* ai)
{
    return FirstGuest(ai);
}

bool KarazhanPhase1Runtime::PrioritizeMoroesGuest(
    PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !IsMoroesActive(ai))
        return false;

    Unit* guest = FirstGuest(ai);
    if (!guest)
        return false;

    Map* map = ai->GetBot()->GetMap();
    if (IsCoordinator(ai) &&
        s_markedGuest[map] != guest->GetObjectGuid())
    {
        Group* group = ai->GetBot()->GetGroup();
        if (group)
        {
            group->SetTargetIcon(
                RAID_ICON_SKULL,
                guest->GetObjectGuid());
            s_markedGuest[map] = guest->GetObjectGuid();
        }
    }

    // Main tank stays on Moroes. Healers retain their class heal target.
    if (IsMainTank(ai))
        SetEncounterTarget(ai, FindTarget(ai, "moroes"));
    else if (!PlayerbotAI::IsHeal(ai->GetBot(), false))
        SetEncounterTarget(ai, guest);

    return false;
}

void ai::karazhan_phase1_detail::ResetMoroes(PlayerbotAI* ai)
{
    if (ai && ai->GetBot() && ai->GetBot()->GetMap())
        s_markedGuest.erase(ai->GetBot()->GetMap());
}
