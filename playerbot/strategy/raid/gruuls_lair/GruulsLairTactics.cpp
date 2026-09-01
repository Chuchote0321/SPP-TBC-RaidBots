#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/HighKingMaulgarEncounter.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulEncounter.h"
#include "playerbot/PlayerbotAI.h"

using namespace ai;

EncounterOverrideResult GruulsLairTactics::Update(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() ||
        ai->GetBot()->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
    {
        return EncounterOverrideResult::NotHandled;
    }

    EncounterTrace::EventOnce(
        ai,
        "GRUULS_LAIR",
        "map-enter",
        "MAP_ENTER",
        "raid=GRUULS_LAIR");

    EncounterOverrideResult result =
        HighKingMaulgarEncounter::Update(ai);

    if (result != EncounterOverrideResult::NotHandled)
        return result;

    result = GruulEncounter::Update(ai);
    if (result != EncounterOverrideResult::NotHandled)
        return result;

    return EncounterOverrideResult::NotHandled;
}
