#pragma once

#include <cstdint>

namespace ai
{
    enum class EncounterOverrideResult : uint8_t
    {
        NotHandled = 0,  // Encounter layer did not consume this tick; run Normal Rotation.
        Handled    = 1,  // Encounter layer executed a hard action; consume this tick.
        BlockNormal= 2   // Encounter layer deliberately blocks Normal Rotation for this tick.
    };

    enum class EncounterActorControl : uint8_t
    {
        None = 0,
        BotAI,
        HumanPlayer
    };

    struct EncounterConstants
    {
        // Gruul's Lair
        static constexpr uint32_t MAP_GRUULS_LAIR = 565;
        static constexpr uint32_t TYPE_MAULGAR_EVENT = 0;
        static constexpr uint32_t TYPE_GRUUL_EVENT   = 1;

        // ScriptDev encounter-state value used by CMaNGOS.
        // Kept local so the PlayerBot module does not need to include a specific
        // dungeon script header.
        static constexpr uint32_t ENCOUNTER_IN_PROGRESS = 1;
        static constexpr uint32_t ENCOUNTER_DONE        = 3;

        // High King Maulgar council entries.
        static constexpr uint32_t NPC_MAULGAR   = 18831;
        static constexpr uint32_t NPC_KROSH     = 18832;
        static constexpr uint32_t NPC_OLM       = 18834;
        static constexpr uint32_t NPC_KIGGLER   = 18835;
        static constexpr uint32_t NPC_BLINDEYE  = 18836;
        static constexpr uint32_t NPC_WILD_FEL_STALKER = 18847;

        // Gruul.
        static constexpr uint32_t NPC_GRUUL = 19044;

        // Maulgar mechanics.
        static constexpr uint32_t SPELL_MAULGAR_WHIRLWIND = 33238;
        static constexpr uint32_t SPELL_KROSH_SPELL_SHIELD = 33054;
        static constexpr uint32_t SPELL_SPELLSTEAL          = 30449;
        static constexpr uint32_t SPELL_BLINDEYE_HEAL       = 33144;
        static constexpr uint32_t SPELL_BLINDEYE_PRAYER     = 33152;

        // Gruul mechanics. 39187 is retained from the upstream Playerbots
        // prototype; 39188 is the dummy spell cast by the current CMaNGOS
        // ScriptDevAI implementation.
        static constexpr uint32_t SPELL_GRUUL_GROUND_SLAM         = 33525;
        static constexpr uint32_t SPELL_GRUUL_GROUND_SLAM_TRIGGER = 39187;
        static constexpr uint32_t SPELL_GRUUL_GROUND_SLAM_DUMMY   = 39188;
        static constexpr uint32_t SPELL_GRUUL_STONED              = 33652;
        static constexpr uint32_t SPELL_GRUUL_SHATTER             = 33654;
        static constexpr uint32_t SPELL_GRUUL_SHATTER_EFFECT      = 33671;
        static constexpr uint32_t SPELL_GRUUL_HURTFUL_PRIMER      = 33812;
        static constexpr uint32_t SPELL_GRUUL_HURTFUL_STRIKE      = 33813;
        static constexpr uint32_t SPELL_GRUUL_CAVE_IN             = 36240;

        // TBC Enslave Demon ranks. Use the highest learned rank.
        static constexpr uint32_t SPELL_ENSLAVE_DEMON_R1 = 1098;
        static constexpr uint32_t SPELL_ENSLAVE_DEMON_R2 = 11725;
        static constexpr uint32_t SPELL_ENSLAVE_DEMON_R3 = 11726;
    };
}
