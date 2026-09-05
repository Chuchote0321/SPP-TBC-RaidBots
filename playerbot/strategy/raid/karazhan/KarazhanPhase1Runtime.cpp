#include "botpch.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Runtime.h"
#include "playerbot/strategy/raid/karazhan/KarazhanPhase1Internal.h"

#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"

#include <algorithm>
#include <limits>

using namespace ai;
using namespace ai::karazhan_phase1_detail;

Unit* KarazhanPhase1Runtime::FindTarget(
    PlayerbotAI* ai,
    const char* name)
{
    if (!ai || !name)
        return nullptr;

    AiObjectContext* context = ai->GetAiObjectContext();
    if (!context)
        return nullptr;

    Value<Unit*>* value =
        context->GetValue<Unit*>("find target", name);
    return value ? value->Get() : nullptr;
}

bool KarazhanPhase1Runtime::IsMoroesActive(PlayerbotAI* ai)
{
    Unit* moroes = FindTarget(ai, "moroes");
    return moroes && moroes->IsAlive();
}

bool KarazhanPhase1Runtime::IsMaidenActive(PlayerbotAI* ai)
{
    Unit* maiden = FindTarget(ai, "maiden of virtue");
    return maiden && maiden->IsAlive();
}

bool KarazhanPhase1Runtime::IsMainTank(PlayerbotAI* ai)
{
    std::vector<Player*> tanks = SortedTanks(ai);
    return ai && ai->GetBot() &&
           !tanks.empty() &&
           tanks.front() == ai->GetBot();
}

bool KarazhanPhase1Runtime::IsAssistTank(PlayerbotAI* ai)
{
    std::vector<Player*> tanks = SortedTanks(ai);
    return ai && ai->GetBot() &&
           tanks.size() > 1 &&
           tanks[1] == ai->GetBot();
}

bool KarazhanPhase1Runtime::IsRangedOrHealer(PlayerbotAI* ai)
{
    return ai && ai->GetBot() &&
        (PlayerbotAI::IsHeal(ai->GetBot(), false) ||
         ai->IsRanged(ai->GetBot(), false));
}

bool KarazhanPhase1Runtime::IsCoordinator(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot())
        return false;

    uint32 lowest = std::numeric_limits<uint32>::max();
    for (Player* player : SortedGroup(ai))
    {
        PlayerbotAI* actorAI = player->GetPlayerbotAI();
        if (!actorAI || actorAI->IsRealPlayer())
            continue;

        lowest = std::min(
            lowest,
            player->GetObjectGuid().GetCounter());
    }

    return lowest != std::numeric_limits<uint32>::max() &&
           lowest == ai->GetBot()->GetObjectGuid().GetCounter();
}

void KarazhanPhase1Runtime::SetEncounterTarget(
    PlayerbotAI* ai,
    Unit* target)
{
    if (!ai || !target || !target->IsAlive())
        return;

    AiObjectContext* context = ai->GetAiObjectContext();
    if (!context)
        return;

    context->GetValue<Unit*>("current target")->Set(target);
    context->GetValue<ObjectGuid>("attack target")
        ->Set(target->GetObjectGuid());
}

void KarazhanPhase1Runtime::Reset(PlayerbotAI* ai)
{
    ResetAttumen(ai);
    ResetMoroes(ai);
    ResetMaiden(ai);
    ResetMovement(ai);
}
