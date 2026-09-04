#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulTriggers.h"

#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.h"
#include "playerbot/PlayerbotAI.h"

using namespace ai;

bool GruulTankPositionTrigger::IsActive()
{
    return GruulRuntime::IsEncounterInProgress(ai) &&
           (GruulRuntime::IsMainTank(ai) ||
            GruulRuntime::IsHurtfulSoaker(ai));
}

bool GruulRangedSpreadTrigger::IsActive()
{
    return GruulRuntime::IsEncounterInProgress(ai) &&
           !GruulRuntime::IsShatterWindow(ai) &&
           GruulRuntime::IsRangedOrHealer(ai) &&
           !GruulRuntime::IsMainTank(ai) &&
           !GruulRuntime::IsHurtfulSoaker(ai);
}

bool GruulIncomingShatterTrigger::IsActive()
{
    return GruulRuntime::IsEncounterInProgress(ai) &&
           GruulRuntime::IsShatterWindow(ai);
}

bool GruulEncounterResetTrigger::IsActive()
{
    return ai && ai->GetBot() &&
           ai->GetBot()->GetMapId() == 565 &&
           !GruulRuntime::IsEncounterInProgress(ai);
}
