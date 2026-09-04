#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulMultipliers.h"

#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/actions/MovementActions.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulActions.h"
#include "playerbot/strategy/raid/gruuls_lair/gruul/GruulRuntime.h"

using namespace ai;

namespace
{
    bool IsNamed(Action* action, const char* name)
    {
        return action && action->getName() == name;
    }

    bool IsGenericTankMovement(Action* action)
    {
        if (!action)
            return false;

        const std::string& name = action->getName();
        return
            name == "tank assist" ||
            name == "dps assist" ||
            name == "dps aoe" ||
            name == "set behind" ||
            name == "follow" ||
            name == "return" ||
            name == "avoid aoe" ||
            name.find("combat formation") != std::string::npos;
    }
}

float GruulDelayBloodlustMultiplier::GetValue(Action* action)
{
    if (!action || !GruulRuntime::ShouldDelayBloodlust(ai))
        return 1.0f;

    const std::string& name = action->getName();
    return (name == "bloodlust" || name == "heroism")
        ? 0.0f
        : 1.0f;
}

float GruulControlMainTankMovementMultiplier::GetValue(Action* action)
{
    if (!action || !GruulRuntime::ShouldLockMainTankMovement(ai))
        return 1.0f;

    if (IsNamed(action, "gruul tank position") ||
        IsNamed(action, "gruul shatter spread"))
    {
        return 1.0f;
    }

    return IsGenericTankMovement(action) ? 0.0f : 1.0f;
}

float GruulShatterMovementMultiplier::GetValue(Action* action)
{
    if (!action || !GruulRuntime::IsShatterWindow(ai))
        return 1.0f;

    if (IsNamed(action, "gruul shatter spread"))
        return 1.0f;

    // In the original mod-playerbots implementation, reach-target spell
    // actions are suppressed together with generic movement while Ground Slam
    // and Shatter positioning are active. CMaNGOS exposes those as named
    // Actions rather than a shared movement base, so retain the semantic rule
    // by rejecting the native "reach ..." action family explicitly.
    const std::string& name = action->getName();
    if (name.find("reach ") == 0)
        return 0.0f;

    return dynamic_cast<MovementAction*>(action) ? 0.0f : 1.0f;
}
