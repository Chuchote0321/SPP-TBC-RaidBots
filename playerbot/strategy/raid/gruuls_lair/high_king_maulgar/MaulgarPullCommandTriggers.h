#pragma once

#include "playerbot/strategy/Trigger.h"

class Player;

namespace ai
{
    // Immediate external-command triggers. They mutate the shared pull state
    // in ExternalEvent because the PREPARING hold blocks the normal Engine.
    class MaulgarPrepareCommandTrigger final : public Trigger
    {
    public:
        explicit MaulgarPrepareCommandTrigger(PlayerbotAI* ai)
            : Trigger(ai, "raid prepare maulgar") {}

        void ExternalEvent(
            std::string param,
            Player* owner = nullptr) override;

        Event Check() override { return Event(); }
    };

    class MaulgarPullCommandTrigger final : public Trigger
    {
    public:
        explicit MaulgarPullCommandTrigger(PlayerbotAI* ai)
            : Trigger(ai, "raid pull maulgar") {}

        void ExternalEvent(
            std::string param,
            Player* owner = nullptr) override;

        Event Check() override { return Event(); }
    };
}
