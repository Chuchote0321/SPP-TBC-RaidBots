#include "botpch.h"
#include "playerbot/strategy/raid/common/EncounterManager.h"
#include "playerbot/strategy/raid/gruuls_lair/GruulsLairTactics.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandController.h"
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

    // Explicit Maulgar prepare/pull commands own the complete NOT_STARTED
    // window. This must run before the legacy encounter router so merely
    // entering map 565 can never arm Misdirection or pull the council.
    EncounterOverrideResult commandResult =
        MaulgarPullCommandController::Update(ai);

    if (commandResult != EncounterOverrideResult::NotHandled)
        return commandResult;

    if (!MaulgarPullCommandController::AllowEncounterDispatch(ai))
        return EncounterOverrideResult::NotHandled;

    EncounterOverrideResult result = GruulsLairTactics::Update(ai);
    if (result != EncounterOverrideResult::NotHandled)
        return result;

    return EncounterOverrideResult::NotHandled;
}
