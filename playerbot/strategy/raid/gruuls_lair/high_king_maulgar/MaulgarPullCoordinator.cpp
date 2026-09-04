#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarPullCoordinator.h"

using namespace ai;

bool MaulgarPullCoordinator::IsConfigured()
{
    // The retired legacy API must never advertise an independently
    // configured pull path. The command controller is authoritative.
    return false;
}

void MaulgarPullCoordinator::Reset(PlayerbotAI* /*ai*/)
{
    // No compatibility state remains.
}

EncounterOverrideResult MaulgarPullCoordinator::UpdatePrePull(
    PlayerbotAI* /*ai*/)
{
    return EncounterOverrideResult::NotHandled;
}

EncounterOverrideResult MaulgarPullCoordinator::UpdateOpening(
    PlayerbotAI* /*ai*/)
{
    return EncounterOverrideResult::NotHandled;
}
