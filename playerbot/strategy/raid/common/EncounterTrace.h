#pragma once

#include "playerbot/strategy/raid/common/EncounterTypes.h"

class PlayerbotAI;

namespace ai
{
    struct EncounterActor;

    // Low-volume encounter telemetry routed through CMaNGOS CustomLogFile.
    // State/assignment helpers deduplicate repetitive AI-tick observations.
    class EncounterTrace
    {
    public:
        static void Event(
            PlayerbotAI* ai,
            const char* encounter,
            const char* eventName,
            const char* format = nullptr,
            ...);

        static void EventOnce(
            PlayerbotAI* ai,
            const char* encounter,
            const char* dedupeKey,
            const char* eventName,
            const char* format = nullptr,
            ...);

        static void EncounterState(
            PlayerbotAI* ai,
            const char* encounter,
            uint32 state);

        static void Assignment(
            PlayerbotAI* ai,
            const char* encounter,
            const char* role,
            const EncounterActor& actor);

        static void ProtectedHuman(
            PlayerbotAI* ai,
            const char* encounter,
            const char* role,
            const EncounterActor& actor);

    private:
        EncounterTrace() = delete;
    };
}