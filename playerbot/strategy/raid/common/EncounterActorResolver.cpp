#include "botpch.h"
#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/PlayerbotAI.h"

using namespace ai;

uint32 EncounterActor::LowGuid() const
{
    return player ? player->GetObjectGuid().GetCounter() : 0;
}

EncounterActor EncounterActorResolver::Find(PlayerbotAI* ai, uint32 lowGuid)
{
    if (!ai || !ai->GetBot() || !lowGuid)
        return EncounterActor();

    Player* bot = ai->GetBot();

    // Fast path for the current bot.
    if (bot->GetObjectGuid().GetCounter() == lowGuid &&
        bot->IsAlive() && ai->IsSafe(bot))
    {
        return EncounterActor(bot, EncounterActorControl::BotAI);
    }

    const std::vector<Player*> players = ai->GetPlayersInGroup();
    for (Player* player : players)
    {
        if (!player || player->GetObjectGuid().GetCounter() != lowGuid)
            continue;

        if (!player->IsAlive() || !ai->IsSafe(player))
            return EncounterActor();

        // Human-controlled players do not receive server-side movement/cast
        // commands from Encounter AI.
        PlayerbotAI* playerAI = player->GetPlayerbotAI();
        if (!playerAI || playerAI->IsRealPlayer())
            return EncounterActor(player, EncounterActorControl::HumanPlayer);

        return EncounterActor(player, EncounterActorControl::BotAI);
    }

    return EncounterActor();
}

EncounterActor EncounterActorResolver::FirstAvailable(
    PlayerbotAI* ai,
    std::initializer_list<uint32> lowGuids)
{
    for (uint32 guid : lowGuids)
    {
        EncounterActor actor = Find(ai, guid);
        if (actor.IsValid())
            return actor;
    }

    return EncounterActor();
}

EncounterActor EncounterActorResolver::PreferredHumanOrFallback(
    PlayerbotAI* ai,
    std::initializer_list<uint32> preferredHumanGuids,
    std::initializer_list<uint32> fallbackBotGuids)
{
    // A protected wow1 actor only suppresses the automated fallback when that
    // actor is actually present, alive, in the same map/instance, and human-controlled.
    for (uint32 guid : preferredHumanGuids)
    {
        EncounterActor actor = Find(ai, guid);
        if (actor.IsValid() && actor.IsHuman())
            return actor;
    }

    for (uint32 guid : fallbackBotGuids)
    {
        EncounterActor actor = Find(ai, guid);
        if (actor.IsValid() && actor.IsBot())
            return actor;
    }

    return EncounterActor();
}

bool EncounterActorResolver::IsCurrentBot(
    PlayerbotAI* ai,
    const EncounterActor& actor)
{
    return ai && ai->GetBot() && actor.IsBot() &&
           ai->GetBot()->GetObjectGuid().GetCounter() == actor.LowGuid();
}
