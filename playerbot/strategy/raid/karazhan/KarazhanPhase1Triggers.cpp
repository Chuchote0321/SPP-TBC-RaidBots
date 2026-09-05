#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Triggers.h"

#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"

using namespace ai;

bool KarazhanAttumenPhaseOneTrigger::IsActive()
{
    return KarazhanPhase1Runtime::IsAttumenPhaseOne(ai);
}

bool KarazhanAttumenPhaseTwoTrigger::IsActive()
{
    return KarazhanPhase1Runtime::IsAttumenPhaseTwo(ai);
}

bool KarazhanAttumenTransitionTrigger::IsActive()
{
    return KarazhanPhase1Runtime::IsCoordinator(ai) &&
           KarazhanPhase1Runtime::IsAttumenPhaseTwo(ai);
}

bool KarazhanMoroesGuestPriorityTrigger::IsActive()
{
    return KarazhanPhase1Runtime::IsMoroesActive(ai) &&
           KarazhanPhase1Runtime::HasMoroesGuest(ai);
}

bool KarazhanMaidenTankPositionTrigger::IsActive()
{
    return ai && ai->GetBot() &&
           PlayerbotAI::IsTank(ai->GetBot(), false) &&
           KarazhanPhase1Runtime::IsMaidenActive(ai);
}

bool KarazhanMaidenRangedPositionTrigger::IsActive()
{
    return KarazhanPhase1Runtime::IsMaidenActive(ai) &&
           KarazhanPhase1Runtime::IsRangedOrHealer(ai) &&
           !KarazhanPhase1Runtime::IsMainTank(ai);
}

bool KarazhanMaidenGroundingTotemTrigger::IsActive()
{
    return ai && ai->GetBot() &&
           ai->GetBot()->getClass() == CLASS_SHAMAN &&
           KarazhanPhase1Runtime::IsMaidenActive(ai);
}

bool KarazhanPhase1ResetTrigger::IsActive()
{
    if (!ai || !ai->GetBot() ||
        ai->GetBot()->GetMapId() !=
            KarazhanPhase1Runtime::MAP_KARAZHAN)
    {
        return false;
    }

    return !KarazhanPhase1Runtime::IsAttumenPhaseOne(ai) &&
           !KarazhanPhase1Runtime::IsAttumenPhaseTwo(ai) &&
           !KarazhanPhase1Runtime::IsMoroesActive(ai) &&
           !KarazhanPhase1Runtime::IsMaidenActive(ai);
}
