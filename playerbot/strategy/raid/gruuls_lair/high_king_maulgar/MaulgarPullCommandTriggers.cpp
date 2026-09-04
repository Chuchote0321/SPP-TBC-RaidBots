#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandTriggers.h"

#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCommandController.h"

using namespace ai;

void MaulgarPrepareCommandTrigger::ExternalEvent(
    std::string,
    Player* owner)
{
    MaulgarPullCommandController::RequestPrepare(ai, owner);
}

void MaulgarPullCommandTrigger::ExternalEvent(
    std::string,
    Player* owner)
{
    MaulgarPullCommandController::RequestPull(ai, owner);
}
