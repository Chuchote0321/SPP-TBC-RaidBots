#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Multipliers.h"

#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/actions/AttackAction.h"
#include "playerbot/strategy/actions/GenericSpellActions.h"
#include "playerbot/strategy/actions/MovementActions.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"

using namespace ai;

namespace
{
    bool IsNamed(Action* action, const char* name)
    {
        return action && action->getName() == name;
    }

    bool IsReachAction(Action* action)
    {
        return action &&
               action->getName().find("reach ") == 0;
    }
}

float KarazhanAttumenTargetingMultiplier::GetValue(Action* action)
{
    if (!action ||
        !KarazhanPhase1Runtime::
            ShouldSuppressAttumenAutomaticTargeting(ai))
    {
        return 1.0f;
    }

    const std::string& name = action->getName();
    return (name == "tank assist" || name == "dps assist")
        ? 0.0f
        : 1.0f;
}

float KarazhanAttumenStackMultiplier::GetValue(Action* action)
{
    if (!action ||
        !KarazhanPhase1Runtime::ShouldKeepAttumenStacked(ai))
    {
        return 1.0f;
    }

    if (IsNamed(action, "karazhan handle attumen phase two") ||
        dynamic_cast<AttackAction*>(action) ||
        dynamic_cast<CastSpellAction*>(action))
    {
        return 1.0f;
    }

    if (IsReachAction(action) ||
        dynamic_cast<MovementAction*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}

float KarazhanAttumenDpsWaitMultiplier::GetValue(Action* action)
{
    if (!action ||
        !KarazhanPhase1Runtime::ShouldWaitForAttumenTank(ai))
    {
        return 1.0f;
    }

    if (dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    if (dynamic_cast<AttackAction*>(action) ||
        dynamic_cast<CastSpellAction*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}

float KarazhanMaidenFormationMultiplier::GetValue(Action* action)
{
    if (!action ||
        !KarazhanPhase1Runtime::ShouldSuppressMaidenFormation(ai))
    {
        return 1.0f;
    }

    if (IsNamed(action, "karazhan position maiden tank") ||
        IsNamed(action, "karazhan position maiden ranged") ||
        IsNamed(action, "set behind"))
    {
        return 1.0f;
    }

    return action->getName().find("combat formation") !=
            std::string::npos
        ? 0.0f
        : 1.0f;
}

float KarazhanMaidenGroundingTotemMultiplier::GetValue(
    Action* action)
{
    if (!action ||
        !KarazhanPhase1Runtime::ShouldReserveMaidenAirTotem(ai))
    {
        return 1.0f;
    }

    const std::string& name = action->getName();
    return
        (name == "wrath of air totem" ||
         name == "grace of air totem" ||
         name == "tranquil air totem" ||
         name == "nature resistance totem" ||
         name == "windfury totem")
        ? 0.0f
        : 1.0f;
}
