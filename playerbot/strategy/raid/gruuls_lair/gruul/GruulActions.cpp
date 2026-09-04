#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.h"

#include "playerbot/strategy/raid/common/EncounterTypes.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulShatterPlanner.h"

using namespace ai;

bool GruulTankPositionAction::Execute(Event& /*event*/)
{
    Creature* gruul = GruulRuntime::FindGruul(ai);
    if (!gruul)
        return false;

    GruulRuntime::SetEncounterTarget(ai, gruul);

    if (GruulRuntime::IsMainTank(ai))
        return GruulRuntime::MaintainMainTankPosition(ai, gruul);

    if (GruulRuntime::IsHurtfulSoaker(ai))
        return GruulRuntime::MaintainHurtfulSoakerPosition(ai, gruul);

    return false;
}

bool GruulTankPositionAction::isUseful()
{
    return GruulRuntime::IsEncounterInProgress(ai) &&
           (GruulRuntime::IsMainTank(ai) ||
            GruulRuntime::IsHurtfulSoaker(ai));
}

bool GruulRangedSpreadAction::Execute(Event& /*event*/)
{
    Creature* gruul = GruulRuntime::FindGruul(ai);
    return gruul && GruulRuntime::MaintainRangedSpread(ai, gruul);
}

bool GruulRangedSpreadAction::isUseful()
{
    return GruulRuntime::IsEncounterInProgress(ai) &&
           !GruulRuntime::IsShatterWindow(ai) &&
           GruulRuntime::IsRangedOrHealer(ai) &&
           !GruulRuntime::IsMainTank(ai) &&
           !GruulRuntime::IsHurtfulSoaker(ai);
}

bool GruulShatterSpreadAction::Execute(Event& /*event*/)
{
    Creature* gruul = GruulRuntime::FindGruul(ai);
    if (!gruul)
        return false;

    const EncounterOverrideResult result =
        GruulShatterPlanner::Update(ai, gruul);

    // Returning false after reaching the reserved slot leaves non-conflicting
    // instant casts and defensive actions available to the normal class engine.
    return result == EncounterOverrideResult::Handled;
}

bool GruulShatterSpreadAction::isUseful()
{
    return GruulRuntime::IsEncounterInProgress(ai) &&
           GruulRuntime::IsShatterWindow(ai);
}

bool GruulEncounterResetAction::Execute(Event& /*event*/)
{
    GruulRuntime::Reset(ai);
    GruulShatterPlanner::Reset(ai);
    return false;
}
