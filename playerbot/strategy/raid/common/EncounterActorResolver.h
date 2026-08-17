#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"
#include <initializer_list>

class Player;

namespace ai
{
    class PlayerbotAI;

    struct EncounterActor
    {
        EncounterActor() : player(nullptr), control(EncounterActorControl::None) {}
        EncounterActor(Player* p, EncounterActorControl c) : player(p), control(c) {}

        Player* player;
        EncounterActorControl control;

        bool IsValid() const { return player != nullptr; }
        bool IsBot() const { return control == EncounterActorControl::BotAI; }
        bool IsHuman() const { return control == EncounterActorControl::HumanPlayer; }
        uint32 LowGuid() const;
    };

    class EncounterActorResolver
    {
    public:
        // Exact actor registry lookup in the current group/instance.
        static EncounterActor Find(PlayerbotAI* ai, uint32 lowGuid);

        // First currently usable actor in the supplied priority order.
        static EncounterActor FirstAvailable(
            PlayerbotAI* ai,
            std::initializer_list<uint32> lowGuids);

        // Prefer a human special actor. Only if no preferred human is present,
        // resolve the first RNDBOT fallback.
        static EncounterActor PreferredHumanOrFallback(
            PlayerbotAI* ai,
            std::initializer_list<uint32> preferredHumanGuids,
            std::initializer_list<uint32> fallbackBotGuids);

        static bool IsCurrentBot(PlayerbotAI* ai, const EncounterActor& actor);
    };
}
