#include "playerbot/playerbot.h"
#include "RaidCoreFacade.h"

using namespace ai;

RaidCoreFacade::RaidCoreFacade(PlayerbotAI* ai) : PlayerbotAIAware(ai)
{
}

Player* RaidCoreFacade::GetBot() const
{
    return ai ? ai->GetBot() : nullptr;
}

AiObjectContext* RaidCoreFacade::GetContext() const
{
    return ai ? ai->GetAiObjectContext() : nullptr;
}

bool RaidCoreFacade::IsUsable(Unit* target, bool requireAlive) const
{
    if (!ai || !target || !target->IsInWorld() || !ai->IsSafe(target))
    {
        return false;
    }

    return !requireAlive || target->IsAlive();
}

bool RaidCoreFacade::HasAura(
    Unit* target,
    uint32 spellId,
    bool checkOwner) const
{
    return IsUsable(target, false) &&
        ai->HasAura(spellId, target, checkOwner);
}

bool RaidCoreFacade::CanCast(
    uint32 spellId,
    Unit* target,
    uint8 effectMask,
    bool checkHasSpell) const
{
    return IsUsable(target, false) &&
        ai->CanCastSpell(spellId, target, effectMask, checkHasSpell);
}

bool RaidCoreFacade::Cast(
    uint32 spellId,
    Unit* target,
    bool waitForSpell) const
{
    return IsUsable(target, false) &&
        ai->CastSpell(spellId, target, nullptr, waitForSpell);
}

bool RaidCoreFacade::SelectTarget(Unit* target) const
{
    if (!IsUsable(target))
    {
        return false;
    }

    AiObjectContext* context = GetContext();
    Player* bot = GetBot();
    if (!context || !bot)
    {
        return false;
    }

    Value<Unit*>* currentTarget =
        context->GetValue<Unit*>("current target");
    Value<ObjectGuid>* attackTarget =
        context->GetValue<ObjectGuid>("attack target");

    if (!currentTarget || !attackTarget)
    {
        return false;
    }

    currentTarget->Set(target);
    attackTarget->Set(target->GetObjectGuid());
    bot->SetSelectionGuid(target->GetObjectGuid());
    return true;
}

bool RaidCoreFacade::Attack(Unit* target) const
{
    return SelectTarget(target) && ExecuteAction("attack", true);
}

bool RaidCoreFacade::ExecuteAction(
    const std::string& actionName,
    bool silent) const
{
    return ai &&
        !actionName.empty() &&
        ai->DoSpecificAction(actionName, Event(), silent);
}

void RaidCoreFacade::InterruptSpell(bool withMeleeAndAuto) const
{
    if (ai)
    {
        ai->InterruptSpell(withMeleeAndAuto);
    }
}

void RaidCoreFacade::StopMoving() const
{
    if (ai)
    {
        ai->StopMoving();
    }
}
