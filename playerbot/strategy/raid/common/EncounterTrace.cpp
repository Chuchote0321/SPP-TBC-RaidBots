#include "botpch.h"
#include "playerbot/strategy/raid/common/EncounterTrace.h"
#include "playerbot/strategy/raid/common/EncounterActorResolver.h"
#include "playerbot/PlayerbotAI.h"

#include <cstdarg>
#include <cstdio>
#include <map>
#include <set>
#include <string>

using namespace ai;

namespace
{
    std::map<Map*, std::set<std::string> > s_onceKeys;
    std::map<Map*, std::map<std::string, uint32> > s_lastEncounterState;

    const char* ControlName(EncounterActorControl control)
    {
        switch (control)
        {
            case EncounterActorControl::BotAI:       return "bot";
            case EncounterActorControl::HumanPlayer: return "human";
            default:                                 return "none";
        }
    }

    std::string FormatDetails(const char* format, va_list args)
    {
        if (!format || !*format)
            return std::string();

        char buffer[2048];
        buffer[0] = '\0';
        vsnprintf(buffer, sizeof(buffer), format, args);
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
    }

    void Emit(
        PlayerbotAI* ai,
        const char* encounter,
        const char* eventName,
        const std::string& details)
    {
        if (!ai || !ai->GetBot() || !encounter || !eventName)
            return;

        Player* bot = ai->GetBot();
        Map* map = bot->GetMap();

        const uint32 instanceId = map ? map->GetInstanceId() : 0;

        sLog.outCustomLog(
            "[RAID_ENCOUNTER] session=%s-%u map=%u instance=%u "
            "bot=%s guid=%u encounter=%s event=%s%s%s",
            encounter,
            instanceId,
            bot->GetMapId(),
            instanceId,
            bot->GetName(),
            bot->GetObjectGuid().GetCounter(),
            encounter,
            eventName,
            details.empty() ? "" : " ",
            details.empty() ? "" : details.c_str());
    }

    bool MarkOnce(
        PlayerbotAI* ai,
        const char* encounter,
        const char* dedupeKey)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap() ||
            !encounter || !dedupeKey)
        {
            return false;
        }

        std::string key(encounter);
        key += ":";
        key += dedupeKey;

        return s_onceKeys[ai->GetBot()->GetMap()].insert(key).second;
    }
}

void EncounterTrace::Event(
    PlayerbotAI* ai,
    const char* encounter,
    const char* eventName,
    const char* format,
    ...)
{
    va_list args;
    va_start(args, format);
    std::string details = FormatDetails(format, args);
    va_end(args);

    Emit(ai, encounter, eventName, details);
}

void EncounterTrace::EventOnce(
    PlayerbotAI* ai,
    const char* encounter,
    const char* dedupeKey,
    const char* eventName,
    const char* format,
    ...)
{
    if (!MarkOnce(ai, encounter, dedupeKey))
        return;

    va_list args;
    va_start(args, format);
    std::string details = FormatDetails(format, args);
    va_end(args);

    Emit(ai, encounter, eventName, details);
}

void EncounterTrace::EncounterState(
    PlayerbotAI* ai,
    const char* encounter,
    uint32 state)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap() || !encounter)
        return;

    Map* map = ai->GetBot()->GetMap();
    std::map<std::string, uint32>& states = s_lastEncounterState[map];

    auto itr = states.find(encounter);
    if (itr != states.end() && itr->second == state)
        return;

    const uint32 oldState =
        itr == states.end() ? 0xFFFFFFFFu : itr->second;

    states[encounter] = state;

    Event(
        ai,
        encounter,
        "ENCOUNTER_STATE",
        "oldState=%u state=%u",
        oldState,
        state);
}

void EncounterTrace::Assignment(
    PlayerbotAI* ai,
    const char* encounter,
    const char* role,
    const EncounterActor& actor)
{
    if (!role)
        return;

    char key[192];
    snprintf(
        key,
        sizeof(key),
        "assignment:%s:%u:%u",
        role,
        actor.LowGuid(),
        uint32(actor.control));

    EventOnce(
        ai,
        encounter,
        key,
        "ROLE_ASSIGN",
        "role=%s actor=%s actorGuid=%u control=%s",
        role,
        actor.IsValid() && actor.player ? actor.player->GetName() : "NONE",
        actor.LowGuid(),
        ControlName(actor.control));
}

void EncounterTrace::ProtectedHuman(
    PlayerbotAI* ai,
    const char* encounter,
    const char* role,
    const EncounterActor& actor)
{
    if (!actor.IsValid() || !actor.IsHuman() || !role)
        return;

    char key[192];
    snprintf(
        key,
        sizeof(key),
        "human:%s:%u",
        role,
        actor.LowGuid());

    EventOnce(
        ai,
        encounter,
        key,
        "HUMAN_PROTECTED",
        "role=%s actor=%s actorGuid=%u suppressMovement=1 suppressCast=1",
        role,
        actor.player ? actor.player->GetName() : "UNKNOWN",
        actor.LowGuid());
}