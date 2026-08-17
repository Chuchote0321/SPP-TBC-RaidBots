#include "botpch.h"
#include "playerbot/strategy/raid/common/EncounterManager.h"
#include "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.h"
#include "playerbot/PlayerbotAI.h"

using namespace ai;

EncounterManager& EncounterManager::Instance()
{
    static EncounterManager instance;
    return instance;
}

EncounterOverrideResult EncounterManager::Update(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return EncounterOverrideResult::NotHandled;

    // v1 registry: exactly one real encounter implementation.
    // Add future encounters here; do not alter Normal Rotation.
    EncounterOverrideResult result = GruulsLairTactics::Update(ai);
    if (result != EncounterOverrideResult::NotHandled)
        return result;

    return EncounterOverrideResult::NotHandled;
}
