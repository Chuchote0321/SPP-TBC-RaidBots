#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/HighKingMaulgarEncounter.h"
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

    // Maulgar keeps its explicit command controller and specialized encounter
    // overlay. Gruul is deliberately not dispatched here: the dragonkiller
    // encounter now runs through the native Strategy/Trigger/Action/Multiplier
    // engine so mandatory movement does not skip the whole class rotation.
    return HighKingMaulgarEncounter::Update(ai);
}
