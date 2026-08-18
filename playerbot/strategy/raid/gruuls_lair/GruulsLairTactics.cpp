#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/HighKingMaulgarEncounter.h"
#include "playerbot/PlayerbotAI.h"

using namespace ai;

EncounterOverrideResult GruulsLairTactics::Update(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || ai->GetBot()->GetMapId() != EncounterConstants::MAP_GRUULS_LAIR)
        return EncounterOverrideResult::NotHandled;

    EncounterTrace::EventOnce(
        ai, "GRUULS_LAIR", "map-enter", "MAP_ENTER",
        "raid=GRUULS_LAIR");

    EncounterOverrideResult result = HighKingMaulgarEncounter::Update(ai);
    if (result != EncounterOverrideResult::NotHandled)
        return result;

    // Gruul will be added only after Maulgar compile + smoke-test closes.
    return EncounterOverrideResult::NotHandled;
}
