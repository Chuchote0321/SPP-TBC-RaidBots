#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Actions.h"

#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"

using namespace ai;

bool KarazhanAttumenPhaseOneAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::HandleAttumenPhaseOne(ai);
}

bool KarazhanAttumenPhaseOneAction::isUseful()
{
    return KarazhanPhase1Runtime::IsAttumenPhaseOne(ai);
}

bool KarazhanAttumenPhaseTwoAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::HandleAttumenPhaseTwo(ai);
}

bool KarazhanAttumenPhaseTwoAction::isUseful()
{
    return KarazhanPhase1Runtime::IsAttumenPhaseTwo(ai);
}

bool KarazhanAttumenTransitionAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::ObserveAttumenTransition(ai);
}

bool KarazhanMoroesGuestPriorityAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::PrioritizeMoroesGuest(ai);
}

bool KarazhanMoroesGuestPriorityAction::isUseful()
{
    return KarazhanPhase1Runtime::IsMoroesActive(ai) &&
           KarazhanPhase1Runtime::HasMoroesGuest(ai);
}

bool KarazhanMaidenTankPositionAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::MaintainMaidenTankPosition(ai);
}

bool KarazhanMaidenTankPositionAction::isUseful()
{
    return ai && ai->GetBot() &&
           PlayerbotAI::IsTank(ai->GetBot(), false) &&
           KarazhanPhase1Runtime::IsMaidenActive(ai);
}

bool KarazhanMaidenRangedPositionAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::MaintainMaidenRangedPosition(ai);
}

bool KarazhanMaidenRangedPositionAction::isUseful()
{
    return KarazhanPhase1Runtime::IsMaidenActive(ai) &&
           KarazhanPhase1Runtime::IsRangedOrHealer(ai) &&
           !KarazhanPhase1Runtime::IsMainTank(ai);
}

bool KarazhanMaidenGroundingTotemAction::Execute(Event& /*event*/)
{
    return KarazhanPhase1Runtime::CastMaidenGroundingTotem(ai);
}

bool KarazhanMaidenGroundingTotemAction::isUseful()
{
    return ai && ai->GetBot() &&
           ai->GetBot()->getClass() == CLASS_SHAMAN &&
           KarazhanPhase1Runtime::IsMaidenActive(ai);
}

bool KarazhanPhase1ResetAction::Execute(Event& /*event*/)
{
    KarazhanPhase1Runtime::Reset(ai);
    return false;
}
