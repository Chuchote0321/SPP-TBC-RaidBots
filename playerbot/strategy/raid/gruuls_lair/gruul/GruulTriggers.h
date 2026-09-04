#pragma once

#include "playerbot/strategy/Trigger.h"
#include "playerbot/strategy/triggers/DungeonTriggers.h"

namespace ai
{
    class GruulsLairEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        GruulsLairEnterDungeonTrigger(PlayerbotAI* ai)
            : EnterDungeonTrigger(
                  ai,
                  "enter gruul's lair",
                  "gruul's lair",
                  565) {}
    };

    class GruulsLairLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        GruulsLairLeaveDungeonTrigger(PlayerbotAI* ai)
            : LeaveDungeonTrigger(
                  ai,
                  "leave gruul's lair",
                  "gruul's lair",
                  565) {}
    };

    class GruulTankPositionTrigger : public Trigger
    {
    public:
        GruulTankPositionTrigger(PlayerbotAI* ai)
            : Trigger(ai, "gruul tank positioning", 1) {}

        bool IsActive() override;
    };

    class GruulRangedSpreadTrigger : public Trigger
    {
    public:
        GruulRangedSpreadTrigger(PlayerbotAI* ai)
            : Trigger(ai, "gruul ranged spread", 1) {}

        bool IsActive() override;
    };

    class GruulIncomingShatterTrigger : public Trigger
    {
    public:
        GruulIncomingShatterTrigger(PlayerbotAI* ai)
            : Trigger(ai, "gruul incoming shatter", 1) {}

        bool IsActive() override;
    };

    class GruulEncounterResetTrigger : public Trigger
    {
    public:
        GruulEncounterResetTrigger(PlayerbotAI* ai)
            : Trigger(ai, "gruul encounter reset", 2) {}

        bool IsActive() override;
    };
}
